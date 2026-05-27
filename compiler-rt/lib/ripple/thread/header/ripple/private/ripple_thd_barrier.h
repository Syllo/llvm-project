//===------------------------ ripple_thd_barrier.h ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal multi-target barrier definitions for Ripple.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ripple/ripple_thd_conventions.h"
#include <stddef.h>

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

// Opaque declaration
#ifndef RIPPLE_THD_BARRIER_IS_DEFINED
typedef struct ripple_thd_barrier_t ripple_thd_barrier_t;
#endif

/// @brief Initializes a barrier
/// @param barrier_id flat index to the barrier among the set of all barriers
/// @param n_participants # participants expected to join the barrier
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) void
_ripple_thd_barrier_init(ripple_thd_barrier_t *barriers, size_t barrier_id,
                         size_t n_participants);

/// @brief Destroys a barrier
RIPPLE_THD_SCALAR
extern void _ripple_thd_barrier_exit(ripple_thd_barrier_t *barriers,
                                     size_t bar_id);

/// @allocates and initializes
RIPPLE_THD_SCALAR
extern ripple_thd_barrier_t *
_ripple_thd_barriers_alloc(void *underlying, unsigned n_thd, unsigned max_dims);

RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) void
_ripple_thd_barriers_dealloc(ripple_thd_barrier_t *base);