//====------------------------- ripple_thd_qurt_sem.h ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Spin-based semaphore implementation. Supports ripple_thd_qurt_pool.
//
//===----------------------------------------------------------------------===//

#pragma once
#include "ripple/ripple_thd_conventions.h"
#include <stdatomic.h>
#include <stddef.h>

typedef struct {
  _Atomic size_t count;
} ripple_thd_qurt_sem_t;

RIPPLE_THD_SCALAR
static inline __attribute__((always_inline)) void
ripple_thd_qurt_sem_init(ripple_thd_qurt_sem_t *sem, size_t initial_count) {
  atomic_init(&sem->count, initial_count);
}

RIPPLE_THD_SCALAR
static inline __attribute__((always_inline)) void
ripple_thd_qurt_sem_post(ripple_thd_qurt_sem_t *sem, size_t count) {
  atomic_fetch_add_explicit(&sem->count, count, memory_order_release);
}

RIPPLE_THD_SCALAR
static inline __attribute__((always_inline)) size_t
ripple_thd_qurt_sem_wait(ripple_thd_qurt_sem_t *sem) {
  for (;;) {
    size_t count = atomic_load_explicit(&sem->count, memory_order_acquire);

    while (count > 0) {
      if (atomic_compare_exchange_weak_explicit(&sem->count, &count, count - 1,
                                                memory_order_acquire,
                                                memory_order_relaxed)) {
        return count - 1;
      }

      /*
       * If compare_exchange fails, count is updated with the current value.
       * Loop again while count > 0.
       */
    }

    /*
     * Spin while unavailable.
     * Optional: insert pause/yield here.
     */
  }
}

RIPPLE_THD_SCALAR
static inline __attribute__((always_inline)) int
ripple_thd_qurt_sem_try_wait(ripple_thd_qurt_sem_t *sem) {
  size_t count = atomic_load_explicit(&sem->count, memory_order_acquire);

  while (count > 0) {
    if (atomic_compare_exchange_weak_explicit(&sem->count, &count, count - 1,
                                              memory_order_acquire,
                                              memory_order_relaxed)) {
      return 1;
    }

    /*
     * On failure, count has been updated with the latest value.
     */
  }

  return 0;
}

RIPPLE_THD_SCALAR
static inline __attribute__((always_inline)) size_t
ripple_thd_qurt_sem_get_count(ripple_thd_qurt_sem_t *sem) {
  return atomic_load_explicit(&sem->count, memory_order_relaxed);
}
