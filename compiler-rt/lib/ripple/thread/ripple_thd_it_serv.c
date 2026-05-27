//====----------------------- ripple_thd_it_serv.cpp ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Iteration server implementation (part of the Ripple thd runtime)
//
//===----------------------------------------------------------------------===//

#include "ripple/ripple_thd_block.h"
#include "ripple/ripple_thd_conventions.h"
#include <assert.h>
#include <ripple/thread.h>
#include <stdatomic.h>
#include <stddef.h>

#ifdef RIPPLE_THD_USE_QHPI
#include <qurt.h> // for qurt_hvx_get_units()
#endif

// _________________ Internal iteration server representation __________________
//
//

/// @brief Finds the iteration server object to be used for the current set of
/// threads along dimension dim.
RIPPLE_THD_SCALAR
// This is pure for each thread. As far as the compiler's concerned, it's pure.
__attribute__((always_inline, pure)) it_serv_t *
_ripple_get_it_serv(ripple_thd_block_t b, unsigned dim) {
  unsigned dims = b->n_dims;
  if (dims == 1)
    return &(b->its[0]);
  unsigned idx = ripple_thd_id(b, dims - 1);
  for (unsigned i = dims - 2; i > 0; i++) {
    idx *= b->shape[i];
    idx += ripple_thd_id(b, i);
  }
  return &(b->its[idx]);
}

// __________________ External iteration server API implementation _____________
//
//

#define _it_serv_overflows(cur, end, inc)                                      \
  ((inc) >= 0 ? (cur >= end) : (cur <= end))

#define _ripple_local_min(a, b) (a) < (b) ? (a) : (b)

#ifdef __cplusplus
#define RTHD_NULL nullptr
#else
#define RTHD_NULL NULL
#endif

/// @brief Thread-safe initializer for an iteration server.
/// Initializes the iteration server associated with dimension dim
/// of thread block \p b.
/// All concurrent users of an iteration server must call init() with the same
/// arguments. Initialization only goes through when all the previous threads
/// have exited the previous instance of this iteration server. Once the first
/// thread initializes the new it_serv, the other threads are free to use it
/// (they still have to call init(), though).
/// Warning: only dim = 0 is supported.
RIPPLE_THD_SCALAR
extern void ripple_it_serv_init(ripple_thd_block_t b, unsigned dim,
                                __INT32_TYPE__ start, __INT32_TYPE__ end,
                                __INT32_TYPE__ stride) {
#if RIPPLE_THD_IT_SERV_DBG
  qurt_printf("entering init\n");
#endif
  assert(dim == 0);
  it_serv_t *serv = _ripple_get_it_serv(b, dim);
  // For now we assume that all the threads we have are involved
  const size_t thid = ripple_thd_id(b, dim);
  size_t local_epoch =
      __atomic_load_n(&(serv->local_epoch[thid]), memory_order_relaxed);
  size_t next_global_epoch = local_epoch + 1;
  // Start the next epoch.
  // First thread to hit this instance of the init() initializes the it_serv
  if (__atomic_compare_exchange_n(
          &(serv->epoch), &local_epoch, next_global_epoch,
          /*weak=*/0, memory_order_acq_rel, memory_order_acquire)) {

// Only update the iteration server when the current one is depleted
#ifdef RIPPLE_THD_QHPI_DEBUG
    unsigned n_spins = 0;
#endif
    // Wait for all the threads to exit the previous instance of this it_serv
    while (__atomic_load_n(&(serv->to_exit), memory_order_acquire) > 0) {
#ifdef RIPPLE_THD_QHPI_DEBUG
      n_spins++;
      if (n_spins == RIPPLE_THD_MAX_SPINS) {
        qurt_printf(
            "[Warning] Thread %u spun %u x waiting for a it serv to deplete\n",
            thid, RIPPLE_THD_MAX_SPINS);
      }
#endif
    }
    serv->local_epoch[thid] = next_global_epoch;
#ifdef RIPPLE_THD_USE_QHPI
    // QHPI can have a number of threads ("slices") that is larger than the
    // number of concurrent threads the underlying system actually supports.
    // In that case, we leave work for the leftover slices, by only visiting
    // our portion of the requested iterations.
    // FIXME: find out how to make this work with several dimensions
    int n_ccur_thd = qurt_hvx_get_units();
    size_t n_total_thd = ripple_thd_get_block_size(b, 0);
    if (n_ccur_thd < n_total_thd) {
      int32_t phase_span = n_ccur_thd * ((end - start) / n_total_thd);
      int32_t phase_id = thid / n_ccur_thd;
      start += phase_id * phase_span;
      end = _ripple_local_min(start + phase_span, end);
    }
#endif
    __atomic_store_n(&(serv->cur), start, memory_order_relaxed);
    __atomic_store_n(&(serv->end), end, memory_order_relaxed);
    __atomic_store_n(&(serv->stride), stride, memory_order_release);

    unsigned n_thd = ripple_thd_get_block_size(b, dim);
    __atomic_store_n(&(serv->to_exit), n_thd, memory_order_release);
    __atomic_store_n(&(serv->to_init), n_thd - 1, memory_order_release);
  } else {
    serv->local_epoch[thid] = next_global_epoch;
    while (__atomic_load_n(&(serv->to_init), memory_order_acquire) == 0)
      ;
    __atomic_fetch_sub(&(serv->to_init), 1, memory_order_acq_rel);
  }
}

#undef _ripple_local_min

/// @brief Marks the exit of this thread from the use of an iteration server,
/// by decrementing the number of threads that have yet to exit.
/// If @p barrier is true, executes a barrier.
/// Otherwise, the next all-thread sync related to this iteration server
// happens the next time we call init() on this iteration server.
/// @param dim the thread block dimension associated with this iteration server
/// @param barrier whether we want the parallel loop served by the iteration
///        server to be followed by a barrier.
RIPPLE_THD_SCALAR
extern void ripple_it_serv_exit(ripple_thd_block_t b, unsigned dim,
                                int barrier) {
  assert(dim == 0);
  it_serv_t *serv = _ripple_get_it_serv(b, dim);
  // If there is an upcoming init() using the same it_serv, it will need to be
  // "maintained," i.e. reinitialized.
  // We allow maintenance to happen as soon as all the threads have exited.
#if RIPPLE_THD_IT_SERV_DBG
  int32_t old_to_exit =
#endif
      // acquire isn't necessary here.
      __atomic_fetch_sub(&(serv->to_exit), 1, memory_order_release);
  // If we need to do a post-barrier, wait,
  // unless we are the last participant to leave this it_serv_t.
#if RIPPLE_THD_IT_SERV_DBG
  qurt_printf("exiting w/ to_exit = %u\n", old_to_exit - 1);
#endif
  if (barrier) {
    ripple_thd_barrier(b, 1u << dim);
  }
}

/// @brief  Returns the next available iteration
///         for the @p serv_id ^th iteration server associated with dimension \p
///         dim.
/// @return an "optional iteration, i.e. a pair {iteration, has_value}"
RIPPLE_THD_SCALAR
extern ripple_opt_it ripple_it_serv_next(ripple_thd_block_t b, unsigned dim) {
  assert(dim == 0);

  it_serv_t *serv = _ripple_get_it_serv(b, dim);
  int32_t end = serv->end;
  int32_t stride = serv->stride;
  int32_t cur_it =
      __atomic_fetch_add(&(serv->cur), serv->stride, memory_order_relaxed);

  int oob = _it_serv_overflows(cur_it, end, stride);
  ripple_opt_it next_it = {cur_it, oob ? 0 : 1};
  return next_it;
}
