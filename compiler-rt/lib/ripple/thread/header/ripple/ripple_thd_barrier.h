//===------------------------ ripple_thd_barrier.h ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Multi-target barrier definitions for Ripple.
//
//===----------------------------------------------------------------------===//

#pragma once
/// @brief barrier definition for each supported runtime

// How long do we wait for a it_serv_t to be depleted before we look again
/// @brief methods to wait or yield if the wait becomes too long
#if (defined(RIPPLE_THD_USE_QHPI) || defined(RIPPLE_THD_USE_QURT))
// TODO: tune these parameters
#define RIPPLE_THD_MAX_SPINS 8192
#else
#warning Assuming a POSIX-compliant underlying thread runtime for Ripple
#define RIPPLE_THD_MAX_SPINS 512
#endif

// ________________________________ QHPI _______________________________________

#if defined(RIPPLE_THD_USE_QHPI)
// With QHPI, we use our own atomics-based barrier
typedef struct {
  int32_t count;
  uint32_t epoch;
} ripple_thd_barrier_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief initializes a barrier
inline void _ripple_thd_barrier_init(ripple_thd_barrier_t *barrier,
                                     size_t n_thd) {
  barrier->count = n_thd;
  barrier->epoch = 0;
}

/// @brief Destroys a barrier
inline void _ripple_thd_barrier_exit(ripple_thd_barrier_t *barrier) {}
#ifdef __cplusplus
}
#endif

// ________________________________ QuRT _______________________________________

#elif defined(RIPPLE_THD_USE_QURT)
#include <qurt_barrier.h>
typedef qurt_barrier_t ripple_thd_barrier_t;
// #include <ripple/ripple_thd_qurt.h>

/// @brief initializes a barrier
inline __attribute__((always_inline)) void
_ripple_thd_barrier_init(ripple_thd_barrier_t *barrier, size_t n_thd) {
  qurt_barrier_init(barrier, n_thd);
}

/// @brief Destroys a barrier
inline __attribute__((always_inline)) void
_ripple_thd_barrier_exit(ripple_thd_barrier_t *barrier) {
  qurt_barrier_destroy(barrier);
}

// ________________________________ pthread ____________________________________

#else // Assume posix
#error "Undefined barrier behavior: use RIPPLE_THD_USE_QURT/RIPPLE_THD_USE_QHPI"
#endif
