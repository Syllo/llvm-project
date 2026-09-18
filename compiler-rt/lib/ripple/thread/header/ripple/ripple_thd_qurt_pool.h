//====------------------------- ripple_thd_qurt_pool.h --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SPMD worker pool for single-Hexagon Ripple threading based on QuRT
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ripple/ripple_thd_qurt.h"
#ifdef __NO_QURT__
#else
#include <qurt_barrier.h>
#include <qurt_thread.h>
#endif
typedef void *(*task_func_t)(void *arg);

typedef struct {
  task_func_t func;
  void *arg;
} task_t;

typedef struct qthd_worker_pool {
  qthd_runtime_t *rt;

  task_t task;
  qurt_barrier_t bar;

  int stop;
} *qthd_worker_pool_t;

RIPPLE_THD_SCALAR
extern void qthd_worker_pool_init(qthd_worker_pool_t pool, qthd_runtime_t *qrt,
                                  int priority, size_t stack_size);
RIPPLE_THD_SCALAR
extern void qthd_worker_pool_exit(qthd_worker_pool_t pool);
RIPPLE_THD_SCALAR
extern void qthd_worker_pool_dispatch(qthd_worker_pool_t pool,
                                      void *(*func)(void *), void *arg);
