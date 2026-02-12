//====------------------------- ripple_thd_qhpi.h -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime for single-NPU Ripple threading based on QHPI
//
//===----------------------------------------------------------------------===//

#pragma once

// ____________________ Fwd declarations for the QHPI runtime __________________

#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if !defined(__HAS_QHPI__)
#ifdef __cplusplus
extern "C" {
#endif
typedef struct QHPI_RuntimeHandle QHPI_RuntimeHandle;
uint32_t qhpi_num_slices(QHPI_RuntimeHandle *);
uint32_t qhpi_slice_number(QHPI_RuntimeHandle *);
extern void *qhpi_sync_block(QHPI_RuntimeHandle *);
#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus
#define RQHPI_RCAST(T, V) reinterpret_cast<T>(V)
#define RQHPI_NULL nullptr
#else
#define RQHPI_RCAST(T, V) (T)(V)
#define RQHPI_NULL NULL
#endif

// _____________________ Private underlying interface runtime __________________

#include <qurt.h>
#ifdef RIPPLE_THD_QHPI_DEBUG
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

#ifndef RIPPLE_THD_USE_QHPI
#define RIPPLE_THD_USE_QHPI
#include "ripple/__ripple_thd.h"
#undef RIPPLE_THD_USE_QHPI
#else
#include "ripple/__ripple_thd.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
/// @brief full number of threads available from the runtime.
inline __attribute__((always_inline)) size_t
_ripple_flat_shape_thd(ripple_thd_block_t block) {
  QHPI_RuntimeHandle *qhpi =
      RQHPI_RCAST(QHPI_RuntimeHandle *, block->underlying);
  return (size_t)qhpi_num_slices(qhpi);
}

// @brief retrieves the base address of the current thread runtime
inline ripple_thd_block_t _ripple_get_thd_block(void *underlying) {
  QHPI_RuntimeHandle *qhpi = RQHPI_RCAST(QHPI_RuntimeHandle *, underlying);
  return RQHPI_RCAST(ripple_thd_block_t, qhpi_sync_block(qhpi));
}

// Nop _ripple_set_thd_block in this implementation of the QHPI layer,
// because we define that the runtime lays in the qhpi_sync_block zone.
// @brief retrieves the base address of the current thread runtime
inline void _ripple_set_thd_block(void *underlying, ripple_thd_block_t block) {}

/// Cleaning up is also not really necessary here.
inline void _ripple_clear_thd_block(ripple_thd_block_t block) {}

// ______________________________ Core API _____________________________________

/// @brief Coordinate of the current thread within the block along dimension dim
static inline __attribute__((always_inline)) size_t
ripple_thd_id(ripple_thd_block_t block, unsigned dim) {
  QHPI_RuntimeHandle *qhpi =
      RQHPI_RCAST(QHPI_RuntimeHandle *, block->underlying);
  return (size_t)qhpi_slice_number(qhpi);
}

/// @brief Spin 1-d barrier with a short (32ns) wait between spins
/// @param b A ripple block (pointer)
inline __attribute__((always_inline)) void
ripple_thd_barrier(ripple_thd_block_t b, unsigned dims) {
  ripple_thd_barrier_t *bar = RQHPI_RCAST(ripple_thd_barrier_t *, &b->barrier);
#ifdef RIPPLE_THD_QHPI_DEBUG
  QHPI_RuntimeHandle *qhpi =
      RQHPI_RCAST(QHPI_RuntimeHandle *, block->underlying);
  if (qhpi_num_slices(qhpi) > _n_hw_thds) {
    qurt_printf("[WARNING] ripple_thd_barrier() used with more slices"
                " than HW threads --> deadlock\n");
  }
#endif
  int32_t n_participants = (int32_t)_ripple_flat_shape_thd(b);

  // Capture the phase we expect to see toggled when the barrier releases.
  uint32_t local_epoch = __atomic_load_n(&(bar->epoch), memory_order_relaxed);

  // Decrement the count; fetch_sub returns the previous value.
  int32_t prev = __atomic_fetch_sub(&(bar->count), 1, memory_order_acq_rel);
  assert(n_participants >= prev);

  local_epoch++;
  if (prev == 1) {
    // We are the last thread to arrive:
    // 1) Reset the counter for the next use of the barrier.
    // 2) Toggle the epoch to release waiters.
    // Use release store so that all prior writes in this epoch
    // become visible to threads in the next epoch.
    __atomic_store(&(bar->count), &n_participants, memory_order_relaxed);
    // Toggle epoch
    __atomic_store(&(bar->epoch), &local_epoch, memory_order_release);
    __atomic_thread_fence(memory_order_acquire);
  } else {
    // Not the last thread: spin until epoch changes.
    // Use acquire loads so that we observe all writes
    // that happened-before the barrier (enforced by last thread’s release).
    // Start by spinning, then sleeping, then yielding
#ifdef RIPPLE_THD_QHPI_DEBUG
    unsigned n_spins = 0;
#endif
    while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
           local_epoch) {
#ifdef RIPPLE_THD_QHPI_DEBUG
      n_spins++;
      if (n_spins == RIPPLE_THD_MAX_SPINS) {
        qurt_printf("[Warning] Thread spun %u times on barrier\n",
                    RIPPLE_THD_MAX_SPINS);
      }
#endif
    }
#if 0 // We'll consider sleeps and yields later
    if (n_spins == RIPPLE_THD_MAX_SPINS) {
    unsigned n_sleeps = 0;
      while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
                 local_epoch &&
             n_sleeps < RIPPLE_THD_MAX_SLEEPS) {
        _ripple_thd_sleep();
        n_sleeps++;
      }
    }
    if (n_sleeps == RIPPLE_THD_MAX_SLEEPS) {
      while (__atomic_load_n(&(bar->epoch), memory_order_acquire) !=
      local_epoch) {
        _ripple_thd_yield();
      }
    }
#endif
  }
  // We've done an acquire in all cases, so no memory operation from after the
  // barrier can move before it.
}

#undef RQHPI_RCAST
#undef RQHPI_NULL

#ifdef __cplusplus
} // extern "C"
#endif