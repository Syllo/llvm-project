//====----------------------- __ripple_thd_block.h ------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations common to multiple targets for ripple/thread.h
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ripple/private/ripple_thd_it_serv.h"
#ifndef RIPPLE_THD_BARRIER_IS_DEFINED
typedef struct ripple_thd_barrier_t ripple_thd_barrier_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Defines the opaque struct from __ripple_vec.h --> ripple_thd_block_t
struct ripple_thread_block {
  /// Backlink to the underlying runtime (optional)
  void *underlying;
  /// Block ID among the ripple_thd_blocks supported by the underlying runtime
  unsigned block_id;
  // These flags determine some of the Ripple thread runtime behavior & needs
  unsigned flags;
  // to_exit and to_init implement phase synchronization
  // between calls to ripple_thd_set_block_shape
  unsigned to_exit;
  unsigned to_init;
  /// Dimension count
  unsigned n_dims;
  size_t shape[RIPPLE_MAX_THD_BLOCK_DIMS];
  // One dimension server per non-0-dimensional thread (dyn dim has to be 0)
  // TODO: make less static and wasteful
  it_serv_t *its;
  size_t bar_ends[RIPPLE_MAX_THD_BLOCK_DIMS_COMBO];
  ripple_thd_barrier_t *barriers;
};

typedef struct ripple_thread_block *ripple_thd_block_t;

#define RIPPLE_THD_BLOCK_IS_DEFINED 1

/// Barrier call on a given barrier object @p bar.
RIPPLE_THD_SCALAR
extern __attribute__((always_inline)) void
_ripple_thd_barrier(ripple_thd_barrier_t *base, unsigned idx);

#ifdef __cplusplus
} // extern "C"
#endif
