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

#include <qurt.h>
#ifdef RIPPLE_THD_QHPI_DEBUG
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

#include <ripple/thread.h>
#define RIPPLE_THD_QHPI_MAX_BLOCKS 1

/**
 * Structure that lives in the QHPI sync block and keeps track of
 * Ripple thread blocks.
 */
typedef struct _ripple_qhpi_sync_block {
  // Used to ensure that only the first slice reaching ripple_thd_init
  // will initialize the Ripple runtime,
  // and the other slices will wait for that runtime to be initialized.
  unsigned lock;

  // Used to allow the last slice reaching ripple_thd_exit to tear down the
  // Ripple runtime
  unsigned unfinished;

  // Number of blocks actually used
  unsigned n_blocks;
  ripple_thd_block_t ripple_rt[RIPPLE_THD_QHPI_MAX_BLOCKS];
} ripple_qhpi_sync_block_t;
