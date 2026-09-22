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

#include "ripple/ripple_thd_conventions.h"
#include <assert.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef __NO_QHPI__
typedef struct QHPI_RuntimeHandle QHPI_RuntimeHandle;
uint32_t qhpi_num_slices(QHPI_RuntimeHandle *);
uint32_t qhpi_slice_number(QHPI_RuntimeHandle *);
extern void *qhpi_sync_block(QHPI_RuntimeHandle *);
#else
#include <qhpi.h>
#endif

#define RIPPLE_THD_QHPI_MAX_BLOCKS 1

/// Binds a thread id with a QHPI runtime object.
typedef struct _qhpi_thd_pair {
  unsigned thd_id;
  QHPI_RuntimeHandle *qhpi;
} qhpi_thd_pair;

// Opaque thd_block_t
typedef struct ripple_thread_block *ripple_thd_block_t;

/**
 * Structure that lives in the QHPI sync block and keeps track of slice ids,
 * number of slices and Ripple thread blocks.
 */
typedef struct _ripple_qhpi_sync_block {
  // Used to ensure that only the first slice reaching ripple_thd_init
  // will initialize the Ripple runtime,
  // and the other slices will wait for that runtime to be initialized.
  unsigned lock;

  // Number of slices managed by this QHPI runtime
  unsigned n_slices;

  // Number of HW threads running the slices
  unsigned n_threads;

  // A memory of all the QHPI_RuntimeHandle pointers, per slice id
  // This lives right after this object in the sync block
  qhpi_thd_pair *qhpi_pairs;

  // Number of blocks actually used
  unsigned n_blocks;
  ripple_thd_block_t ripple_rts;

} qhpi_runtime_t;

/// @brief Size (in bytes) used by the multi-threading Ripple runtime, which
/// needs to be reserved by QHPI if the operator is
/// multi-threaded/self-threaded. For single-threaded operators, no runtime
/// allocation / allocation is necessary.
/// @param n_slices Number of QHPI slices the operator/kernel executes on.
/// @param max_dims the maximum number of dimensions used in a thread block in
///                 the kernel.
/// @param n_blocks number of distinct Ripple blocks used in this kernel
///                  (usually 1).
/// @param flags a bitset to indicate the special thread parallelism features
///               to be used in the operator/kernel. These are the same flags
///               passed to ripple_thd_init (RIPPLE_THD_USE_BAR,
///               RIPPLE_THD_USE_DYN etc.)
RIPPLE_THD_SCALAR
extern size_t ripple_qhpi_runtime_size(unsigned n_slices, unsigned max_dims,
                                       unsigned n_blocks, unsigned flags);