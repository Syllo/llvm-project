//====------------------------- ripple_thd_qurt_pool.c --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// SPMD worker pool for single-Hexagon Ripple threading based on QuRT.
// Based on spin-waits.
//
//===----------------------------------------------------------------------===//

#include "ripple/ripple_thd_qurt_pool.h"
#include <stdlib.h>

#ifdef __NO_QURT__
typedef union qurt_barrier qurt_barrier_t;
int qurt_barrier_init(qurt_barrier_t *barrier, unsigned int threads_total);
int qurt_barrier_destroy(qurt_barrier_t *barrier);
int qurt_barrier_wait(qurt_barrier_t *barrier);
#endif

// Worker thread entry function
static void qthd_worker_thread_main(void *arg) {
  qthd_worker_pool_t pool = (qthd_worker_pool_t)arg;
  int err = qurt_hvx_lock(QURT_HVX_MODE_128B);
  if (err != QURT_EOK) {
    qurt_printf("FAILED to lock HVX\n");
    return; // Don't call func without HVX
  }

  while (1) {
    // Wait for the parent thread to tell us that work is ready
    qurt_barrier_wait(&(pool->bar));
    if (pool->stop) {
      break;
    } else {
      // Execute task
      task_func_t func = pool->task.func;
      void *task_arg = pool->task.arg;
      if (func) {
        func(task_arg);
      }
    }
    // Wait for everyone to be ready to exit
    qurt_barrier_wait(&(pool->bar));
  }

  err = qurt_hvx_unlock();
  if (err != QURT_EOK) {
    qurt_printf("FAILED to unlock HVX\n");
  }

  qurt_thread_exit(0);
}

void qthd_worker_pool_init(qthd_worker_pool_t pool, qthd_runtime_t *rt,
                           int priority, size_t stack_size) {
  pool->rt = rt;
  pool->stop = 0;
  // This is a parent-children barrier
  // TODO: to be robust to cases when thread_creates fail, create an initial
  // sync with all the children thread and initialize the barrier when the
  // thread count is known.
  qurt_barrier_init(&(pool->bar), rt->n_thd + 1);

  for (int t = 0; t < rt->n_thd; t++) {
    qurt_thread_attr_t attr;

    qurt_thread_attr_init(&attr);
    qurt_thread_attr_set_stack_size(&attr, stack_size);
    qurt_thread_attr_set_stack_addr(&attr,
                                    (unsigned long long *)rt->stack[t]);
    qurt_thread_attr_set_priority(&attr, priority);
    qurt_thread_attr_set_name(
        &attr, &rt->names[QURT_RIPPLE_MAX_THD_NAME_SIZE * t]);
    qurt_thread_attr_set_detachstate(&attr, QURT_THREAD_ATTR_CREATE_JOINABLE);
#ifdef RIPPLE_THD_QURT_DEBUG
    qurt_printf("Stack size=%u, stack_addr=0x%x, prio=%u, name=%s\n",
                stack_size, rt->stack[t], priority,
                &rt->names[QURT_RIPPLE_MAX_THD_NAME_SIZE * t]);
#endif

    int ret = qurt_thread_create(&rt->threads[t], &attr,
                                 qthd_worker_thread_main, (void *)pool);
    if (ret != QURT_EOK) {
#ifdef RIPPLE_THD_QURT_DEBUG
      switch (ret) {
      case QURT_EMEM:
        qurt_printf("Insufficient mem\n");
        break;
      case QURT_EINVALID:
        qurt_printf("Invalid parms\n");
        break;
      case QURT_EFAILED:
        qurt_printf("General failure\n");
        break;
      case QURT_ENOTHREAD:
        qurt_printf("Max threads exceeded\n");
        break;
      default:
        qurt_printf("Unknown error code (%u)\n", ret);
      }
#endif
      qurt_printf("ripple_thd_qurt_pool: Failed to create thread\n");
    }
  }
}

void qthd_worker_pool_dispatch(qthd_worker_pool_t pool, task_func_t func,
                               void *arg) {
  pool->task.func = func;
  pool->task.arg = arg;

  // Wake workers
  qurt_barrier_wait(&(pool->bar));

  // Wait for all workers to finish
  qurt_barrier_wait(&(pool->bar));
}

void qthd_worker_pool_exit(qthd_worker_pool_t pool) {
  pool->stop = 1;
  qurt_barrier_wait(&(pool->bar));

  int status;
  for (unsigned i = 0; i < pool->rt->n_thd; i++) {
    qurt_thread_join(pool->rt->threads[i], &status);
  }
  qurt_barrier_destroy(&(pool->bar));
}
