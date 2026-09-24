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

#ifdef RIPPLE_THD_QURT_DEBUG
#define SHOW_SEM(sem)                                                          \
  qurt_printf("line %u, sem count: %u\n", __LINE__, (sem)->count);
#else
#define SHOW_SEM(sem) (void)0
#endif

extern void _set_sw_id(void *underlying, size_t sw_id);

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
    size_t sw_id = ripple_thd_qurt_sem_wait(&(pool->todo));
    _set_sw_id(pool->rt, sw_id);
    SHOW_SEM(&pool->todo);
    int local_stop = pool->stop; // protect "stop" against concurrent changes
    if (!local_stop) {
#ifdef RIPPLE_THD_QURT_DEBUG
      qurt_printf("Thd %p running %p(%p)\n", qurt_thread_get_id(),
                  pool->task.func, pool->task.arg);
#endif
      // Execute task
      task_func_t func = pool->task.func;
      void *task_arg = pool->task.arg;
      if (func) {
        func(task_arg);
      }
    }
    ripple_thd_qurt_sem_post(&(pool->done), 1);
    SHOW_SEM(&pool->done);
    if (local_stop)
      break;
    // Wait for everyone to be ready to exit
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

  ripple_thd_qurt_sem_init(&(pool->todo), 0);
  ripple_thd_qurt_sem_init(&(pool->done), 0);

  for (unsigned t = 0; t < rt->n_thd; t++) {
    qurt_thread_attr_t attr;

    qurt_thread_attr_init(&attr);
    qurt_thread_attr_set_stack_size(&attr, stack_size);
    if (((size_t)rt->stack[t]) % 8 != 0) {
      qurt_printf("[Warning] Thread stack for thread %u (%p) not aligned!\n", t,
                  rt->stack[t]);
    }
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
  unsigned n_thd = pool->rt->n_thd;

  // Tell workers there is work to do
  ripple_thd_qurt_sem_post(&pool->todo, n_thd);
  SHOW_SEM(&pool->todo);

  // Wait for all the tasks to be done
  for (unsigned t = 0; t < n_thd; ++t) {
    ripple_thd_qurt_sem_wait(&pool->done);
    SHOW_SEM(&pool->done);
  }
}

void qthd_worker_pool_exit(qthd_worker_pool_t pool) {
  pool->stop = 1;
  // OPT: we don't really have to wait for the "stop" task to be done,
  // since we're doing a join right after this.
  qthd_worker_pool_dispatch(pool, NULL, NULL);

  int status;
  for (unsigned i = 0; i < pool->rt->n_thd; i++) {
    qurt_thread_join(pool->rt->threads[i], &status);
  }
}
