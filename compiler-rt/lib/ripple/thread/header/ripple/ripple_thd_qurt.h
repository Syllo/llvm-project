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

#include <qurt.h>
#include <qurt_thread.h>
#include <stdlib.h>

#ifdef RIPPLE_THD_QURT_DEBUG
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

#ifndef RIPPLE_THD_USE_QURT
#define RIPPLE_THD_USE_QURT
#include "ripple/__ripple_thd.h"
#undef RIPPLE_THD_USE_QURT
#else
#include "ripple/__ripple_thd.h"
#endif

/** A wrapper for pthread contexts that includes an extra thread id. */
typedef struct _context_with_id {
  void *(*func)(void *);
  void *args;
} context_f_args_t;

// Default QNN-HTP stack size
#define RIPPLE_THD_QURT_STACK_SIZE 262144
#define QURT_RIPPLE_MAX_THD_NAME_SIZE 8

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
  /// @brief thread stacks (one per thread)
  char **stack;
  // A place to put the return status returned by all threads.
  int *return_status;
  /// @brief  A link to the Ripple runtime associated with this one
  void *ripple_rt;
} qthd_runtime_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Creates a qthread-based environment that runs @p n_thd threads,
/// Which can be used to create a Ripple block.
extern qthd_runtime_t *qthd_runtime_init(unsigned n_thd);

/// @brief Tears down the qthread-based environment
extern void qthd_runtime_exit(qthd_runtime_t *rt);

#ifdef __cplusplus
}
#endif

decl_thread_methods(qthd);

// _________________________ Thread spawning ___________________________________
//
//

#include <hexagon_protos.h>

/// A wrapper function that acquires an HVX resource before calling
// the intended function.
static inline void _qthd_init_hvx(void *void_args_with_id) {
  context_f_args_t *args_with_id = (context_f_args_t *)void_args_with_id;
  void *(*func)(void *) = args_with_id->func;
  void *args = args_with_id->args;
  int err = qurt_hvx_lock(QURT_HVX_MODE_128B);
  if (err != QURT_EOK) {
    qurt_printf("FAILED to lock HVX\n");
  }

#ifdef RIPPLE_THD_QURT_DEBUG
  qurt_thread_t const id = qurt_thread_get_id();
  qurt_printf("Creating thread %x\n", id);
#endif

  func(args);
  err = qurt_hvx_unlock();
  if (err != QURT_EOK) {
    qurt_printf("FAILED to unlock HVX\n");
  }
}

/// @brief Starts a QuRT thread on thread @p thid and runs f(args)
/// @param qthd the QuRT thread object on which f(args) will run
/// @param attr qurt_thread attributes (output)
/// @return error code from qurt_thread_create
static inline __attribute__((always_inline)) int
qthd_create_with_id(qurt_thread_t *qthd, void *(*f)(void *), void *args,
                    qthd_runtime_t *rt, unsigned thid) {
  int ret = 5;
  qurt_thread_attr_t attr;
  // Maintain caller thread priority
  // qurt_thread_t const id = qurt_thread_get_id();
  // int const priority = qurt_thread_get_priority(id);
  const unsigned name_len = QURT_RIPPLE_MAX_THD_NAME_SIZE;

  qurt_thread_attr_init(&attr);
  qurt_thread_attr_set_stack_size(&attr, RIPPLE_THD_QURT_STACK_SIZE);
  qurt_thread_attr_set_stack_addr(&attr, (unsigned long long *)rt->stack[thid]);
  qurt_thread_attr_set_priority(&attr, QURT_THREAD_ATTR_PRIORITY_DEFAULT);
  qurt_thread_attr_set_name(&attr, (char *)&rt->names[name_len * thid]);
  qurt_thread_attr_set_detachstate(&attr, QURT_THREAD_ATTR_CREATE_JOINABLE);
#ifdef RIPPLE_THD_QURT_DEBUG
  qurt_printf("Stack size=%u, stack_addr=0x%x, prio=%u, name=%s\n",
              RIPPLE_THD_QURT_STACK_SIZE, rt->stack[thid], 100,
              &rt->names[name_len * thid]);
#endif

  rt->ctxs[thid].func = f;
  rt->ctxs[thid].args = args;
  ret = qurt_thread_create(qthd, &attr, _qthd_init_hvx,
                           (void *)&(rt->ctxs[thid]));
#ifdef RIPPLE_THD_QURT_DEBUG
  if (ret != QURT_EOK)
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
  return ret;
}

/// @brief Helper macro that initiates a fork-join, multi-threaded execution of
/// a function with a given set of arguments.
#define ripple_thd_call(block, func, args)                                     \
  {                                                                            \
    size_t n_thd = _ripple_flat_shape_thd(block);                              \
    qthd_runtime_t *rt = (qthd_runtime_t *)block->underlying;                  \
    for (unsigned t = 0; t < (n_thd); ++t) {                                   \
      qthd_create_with_id(&rt->threads[t], func, args, rt, t);                 \
    }                                                                          \
    for (unsigned t = 0; t < (n_thd); ++t) {                                   \
      qurt_thread_join(rt->threads[t], &rt->return_status[t]);                 \
    }                                                                          \
  }

// _____________________________ Core API ______________________________________
//
//

/// @brief Encodes an integer @p id into a string pointed at @p nm_ptr
/// @p 0 <= id < 100.
#define _qurt_ripple_thd_write_id(nm_ptr, id)                                  \
  ({                                                                           \
    char ascii_0 = 48;                                                         \
    char *cbase = (char *)(nm_ptr);                                            \
    if ((id) < 10) {                                                           \
      *cbase = (char)ascii_0 + (char)(id);                                     \
      *(cbase + 1) = '\0';                                                     \
    } else {                                                                   \
      *cbase = (char)ascii_0 + (char)((id) / 10);                              \
      *(cbase + 1) = (char)ascii_0 + (char)((id) % 10);                        \
      *(cbase + 2) = '\0';                                                     \
    }                                                                          \
  })

/// @brief Decodes an integer @p id from a string pointed at @p nm_ptr
/// @p 0 <= id < 100.
#define _qurt_ripple_thd_read_id(nm_ptr)                                       \
  ({                                                                           \
    char ascii_0 = 48;                                                         \
    char *cbase = (char *)(nm_ptr);                                            \
    char c0 = *cbase;                                                          \
    char c1 = *(cbase + 1);                                                    \
    int id = 0;                                                                \
    if (c1 == '\0') {                                                          \
      id = (size_t)(c0 - ascii_0);                                             \
    } else {                                                                   \
      id = (c0 - ascii_0) * 10 + (c1 - ascii_0);                               \
    }                                                                          \
    id;                                                                        \
  })

/// @brief Ripple ID for QURT threads.
/// TODO: Only supports one dimension for now
static inline __attribute__((always_inline)) size_t
ripple_thd_id(ripple_thd_block_t thdb, unsigned dim) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  return _qurt_ripple_thd_read_id(thd_name);
}

static inline void ripple_thd_barrier(ripple_thd_block_t block, unsigned dims) {
  qurt_barrier_t *bar = (qurt_barrier_t *)&(block->barrier);
  qurt_barrier_wait(bar);
}
