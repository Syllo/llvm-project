//====------------------------- ripple_thd_qurt.h -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Runtime for single-NPU Ripple threading based on QuRT
//
//===----------------------------------------------------------------------===//

#pragma once

// ___________________________ Underlying runtime ______________________________
//
//

#include "ripple/ripple_thd_conventions.h"
#include <qurt.h>
#include <qurt_thread.h>
#include <stdlib.h>

#ifdef RIPPLE_THD_QURT_DEBUG
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

#include <ripple/thread.h>

/** A wrapper for pthread contexts that includes an extra thread id. */
typedef struct _context_with_id {
  void *(*func)(void *);
  void *args;
} context_f_args_t;

// Default QNN-HTP stack size
#define RIPPLE_THD_QURT_STACK_SIZE 262144
#define QURT_RIPPLE_MAX_THD_NAME_SIZE 8

typedef struct qthd_worker_pool *qthd_worker_pool_t;

/* Runtime object for pthreads */
typedef struct qthd_runtime {
  /// @brief  # threads in this environment
  unsigned n_thd;
  /// @brief We remember one pthread_t per thread
  qurt_thread_t *threads;
  /// @brief attribute for each thread
  qurt_thread_attr_t *attrs;
  /// Contexts passed to each thread
  context_f_args_t *ctxs;
  /// @brief an array to store the names (really, the IDs) of the threads
  char names[QURT_RIPPLE_MAX_THD_NAME_SIZE * QURT_MAX_HTHREAD_LIMIT];
  /// Software IDs assigned to the worker threads for the current dispatch.
  size_t *sw_ids;
  /// @brief thread stacks (one per thread)
  char **stack;
  // A place to put the return status returned by all threads.
  int *return_status;
  // # ripple thd blocks associated with this underlying runtime
  unsigned n_blocks;
  // Worker pool for lightweight multi-threaded calls
  qthd_worker_pool_t pool;
  /// @brief  A link to the Ripple runtime associated with this one
  ripple_thd_block_t ripple_rt[RIPPLE_THD_QURT_MAX_BLOCKS];
} qthd_runtime_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Creates a qthread-based environment that runs @p n_thd threads,
/// Which can be used to create a Ripple block.
extern qthd_runtime_t *qthd_runtime_init(unsigned n_thd);

/// @brief Tears down the qthread-based environment
extern void qthd_runtime_exit(qthd_runtime_t *rt);

extern void *_ripple_thd_underlying(ripple_thd_block_t block);

RIPPLE_THD_SCALAR
extern void qthd_worker_pool_dispatch(qthd_worker_pool_t pool,
                                      void *(*func)(void *), void *arg);

#ifdef __cplusplus
}
#endif

/// @brief Helper macro that initiates a fork-join, multi-threaded execution of
/// a function with a given set of arguments.
#define ripple_thd_call(block, func, args)                                     \
  {                                                                            \
    qthd_runtime_t *rt = (qthd_runtime_t *)_ripple_thd_underlying(block);      \
    qthd_worker_pool_dispatch(rt->pool, func, args);                           \
  }
