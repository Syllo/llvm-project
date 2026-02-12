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

#include <assert.h>
#include <ripple/ripple_thd.h>
#include <ripple/ripple_thd_it_serv.h>
#include <stdatomic.h>
#include <stddef.h>

#ifdef RIPPLE_THD_USE_QHPI
#include <qurt.h> // for qurt_hvx_get_units()
#endif

// _________________ Internal iteration server representation __________________
//
//

/// @brief Creates an iteration server object representing the current
#define _ripple_get_it_serv(b, serv_id)                                        \
  ({                                                                           \
    assert(serv_id < RIPPLE_IT_SERV_COUNT);                                    \
    &(b->its[serv_id]);                                                        \
  })

// __________________ External iteration server API implementation _____________
//
//

// Special # participants that indicates that we're resetting an it_serv_t
#define RIPPLE_THD_IT_SERV_RESETTING -1
#define RIPPLE_THD_IT_SERV_SLEEP_NTRIALS 8

#define _it_serv_overflows(cur, end, inc)                                      \
  ((inc) >= 0 ? (cur >= end) : (cur <= end))

#define _ripple_local_min(a, b) (a) < (b) ? (a) : (b)

#ifdef __cplusplus
#define RTHD_NULL nullptr
#else
#define RTHD_NULL NULL
#endif

/// @brief Thread-safe initializer for an iteration server.
/// Initializes the @p serv_id^th iteration server and increments the epoch if:
/// - another thread hasn't already done so
/// - the previous instance of this iteration server is depleted.
///
/// All concurrent users of an iteration server are supposed to use the same
/// initializer.
/// Note: Some calculations may still be running from the previous instance
/// with already-served iterations. This is desired in the no-barrier behavior.
/// If a barrier was used in the previous it_serv_exit, we also know
/// that all the computations associated with these iterations were _completed_.
extern void ripple_it_serv_init(ripple_thd_block_t b, unsigned int serv_id,
                                int32_t start, int32_t end, int32_t stride) {
  it_serv_t *serv = _ripple_get_it_serv(b, serv_id);
  // For now we assume that all the threads we have are involved
  const int32_t resetting = RIPPLE_THD_IT_SERV_RESETTING;
  int32_t normal = 0;
  size_t thid = ripple_thd_id(b, 0);
  size_t local_epoch = serv->local_epoch[thid];
  size_t global_epoch = __atomic_load_n(&(serv->epoch), memory_order_acquire);
  // Nothing to do if our thread is behind the epochs
  if ((local_epoch < global_epoch) ||
      ((global_epoch < local_epoch) &&
       (local_epoch - global_epoch > SIZE_MAX / 2))) {
    // catch up. There is a constraint that the number of parallel_thd loops
    // doesn't depend upon ripple_thd_id.
    serv->local_epoch[thid]++;
    return;
  }
  // Make sure that the previous instance of this iteration server is depleted
  // before starting
  const int32_t old_end = __atomic_load_n(&(serv->end), memory_order_acquire);
  const int32_t old_inc =
      __atomic_load_n(&(serv->stride), memory_order_acquire);
#ifdef RIPPLE_THD_QHPI_DEBUG
  unsigned n_spins = 0;
#endif
  for (;;) {
    const int32_t old_cur = __atomic_load_n(&(serv->cur), memory_order_acquire);
    if (_it_serv_overflows(old_cur, old_end, old_inc))
      break;
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
    for (; n_sleeps < RIPPLE_THD_MAX_SLEEPS; n_sleeps++) {
      const int32_t old_cur =
          __atomic_load_n(&(serv->cur), memory_order_acquire);
      if (_it_serv_overflows(old_cur, old_end, old_inc))
        break;
      _ripple_thd_sleep();
    }
  }
  if (n_sleeps == RIPPLE_THD_MAX_SLEEPS) {
    for (;;) {
      const int32_t old_cur =
      __atomic_load_n(&(serv->cur), memory_order_acquire);
      if (_it_serv_overflows(old_cur, old_end, old_inc))
      break;
      _ripple_thd_yield();
    }
  }
  local_epoch++;
#endif
#ifdef RIPPLE_THD_USE_QHPI
  // QHPI claim to have a number of threads ("slices") that is larger than the
  // number of concurrent threads the underlying system actually supports.
  // In that case, we leave work for the leftover "threads", by only visiting
  // our portion of the requested iterations.
  int n_ccur_thd = qurt_hvx_get_units();
  size_t n_total_thd = ripple_thd_get_block_size(b, 0);
  if (n_ccur_thd < n_total_thd) {
    int32_t phase_span = n_ccur_thd * ((end - start) / n_total_thd);
    int32_t phase_id = thid / n_ccur_thd;
    start += phase_id * phase_span;
    end = _ripple_local_min(start + phase_span, end);
  }
#endif
  // One of the ready threads updates the it_serv
  if (__atomic_compare_exchange_n(&(serv->mode), &normal, resetting,
                                  /*weak=*/0, memory_order_acq_rel,
                                  memory_order_acquire)) {
    // We're in charge of resetting the iteration server. Let's do it!
    serv->cur = start;
    serv->end = end;
    serv->stride = stride;
    serv->n_pre_exits = 0;
    __atomic_store_n(&(serv->epoch), local_epoch, memory_order_release);
    __atomic_store_n(&(serv->mode), normal, memory_order_relaxed);
  } else { // the other ones spin
    while (__atomic_load_n(&(serv->epoch), memory_order_acquire) != local_epoch)
      ;
  }
  serv->local_epoch[thid]++;
}

#undef _ripple_local_min

/// @brief Marks the exit of this thread from the use of an iteration server,
/// by decrementing the number of participants.
/// If @p barrier is true,
/// In the latter case, atomically resets the iteration server
/// to a trivially depleted one.
/// @param serv_id the index of the iteration server to use.
/// A runtime like this one uses two.
/// @param barrier whether we want the parallel loop served by the iteration
/// server
///        to be followed by a barrier.
extern void ripple_it_serv_exit(ripple_thd_block_t b, unsigned int serv_id,
                                int barrier) {
  it_serv_t *serv = _ripple_get_it_serv(b, serv_id);

  // If this is an early exit, increment the number of early exits
  // If all the threads have early-exited,
  // count my thread out of the participants
  const int32_t end = __atomic_load_n(&(serv->end), memory_order_acquire);
  const int32_t inc = __atomic_load_n(&(serv->stride), memory_order_acquire);
  const int32_t cur = __atomic_load_n(&(serv->cur), memory_order_acquire);
  if (!_it_serv_overflows(cur, end, inc)) {
    int32_t prev =
        __atomic_fetch_add(&(serv->n_pre_exits), 1, memory_order_acq_rel);
    // if all the threads have early-exited, pretend the loop is over,
    // so the next cal to ripple_it_serv_init can start.
    if (prev == ripple_thd_get_block_size(b, 0) - 1) {
      __atomic_store_n(&(serv->cur), end, memory_order_release);
    }
  }

  // If we need to do a post-barrier, wait,
  // unless we are the last participant to leave this it_serv_t.
  if (barrier) {
    ripple_thd_barrier(b, 1u);
  }
}

/// @brief  Returns the next available iteration
///         for the @   p serv_id ^th iteration server.
/// @return an "optional iteration, i.e. a pair {iteration, has_value}"
extern ripple_opt_it ripple_it_serv_next(ripple_thd_block_t b,
                                         unsigned int serv_id) {
  it_serv_t *serv = _ripple_get_it_serv(b, serv_id);
  int32_t end = serv->end;
  int32_t stride = serv->stride;
  int32_t cur_it =
      __atomic_fetch_add(&(serv->cur), serv->stride, memory_order_acq_rel);

  int oob = _it_serv_overflows(cur_it, end, stride);
  ripple_opt_it next_it = {cur_it, oob ? 0 : 1};
  return next_it;
}
