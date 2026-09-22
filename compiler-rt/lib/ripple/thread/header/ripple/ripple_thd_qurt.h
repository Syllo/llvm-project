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

#ifdef __NO_QURT__
#define QURT_THREAD_ATTR_PRIORITY_DEFAULT 254 /**< Priority.*/
#define QURT_THREAD_ATTR_CREATE_JOINABLE 1U   /**< Create a joinable thread. */

typedef unsigned int qurt_thread_t;
typedef struct _qurt_thread_attr qurt_thread_attr_t;
void qurt_thread_get_name(char *name, unsigned char max_len);
qurt_thread_t qurt_thread_get_id(void);
int qurt_thread_create(qurt_thread_t *thread_id, qurt_thread_attr_t *attr,
                       void (*entrypoint)(void *), void *arg);
int qurt_thread_join(unsigned int tid, int *status);

void qurt_thread_attr_init(qurt_thread_attr_t *attr);
void qurt_thread_attr_set_name(qurt_thread_attr_t *attr, const char *name);
void qurt_thread_attr_set_priority(qurt_thread_attr_t *attr,
                                   unsigned short priority);
void qurt_thread_attr_set_detachstate(qurt_thread_attr_t *attr,
                                      unsigned short detachstate);
void qurt_thread_attr_set_stack_size(qurt_thread_attr_t *attr,
                                     unsigned int stack_size);
void qurt_thread_attr_set_stack_addr(qurt_thread_attr_t *attr,
                                     void *stack_addr);
typedef enum {
  QURT_HVX_MODE_64B = 0, /**< HVX mode of 64 bytes */
  QURT_HVX_MODE_128B = 1 /**< HVX mode of 128 bytes */
} qurt_hvx_mode_t;
int qurt_hvx_unlock(void);
int qurt_hvx_lock(qurt_hvx_mode_t lock_mode);
#else
#include <qurt.h>
#include <qurt_thread.h>
#endif

#ifdef RIPPLE_THD_QURT_DEBUG
int qurt_printf(const char *msg, ...);
#define print(x, ...) qurt_printf(x, __VA_ARGS__)
#else
#define print(x, ...)
#endif

#include <ripple/hexagon.h>
#include <ripple/thread.h>
#include <stdlib.h>

/** A wrapper for pthread contexts that includes an extra thread id. */
typedef struct _context_with_id {
  void *(*func)(void *);
  void *args;
} context_f_args_t;

// Default QNN-HTP stack size
#define RIPPLE_THD_QURT_STACK_SIZE 262144
#define QURT_RIPPLE_MAX_THD_NAME_SIZE 8
#define RIPPLE_THD_QURT_MAX_BLOCKS 2

typedef struct qthd_info {
  /// @brief We remember one pthread_t per thread
  qurt_thread_t thread;
  /// Contexts passed to each thread
  context_f_args_t ctx;
  /// @brief an array to store the names (really, the IDs) of the threads
  char name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  /// @brief thread stacks (one per thread)
  char *stack;
  // A place to put the return status returned by all threads.
  int return_status;
} qthd_info_t;

/* Runtime object for pthreads */
typedef struct qthd_runtime {
  /// @brief  # threads in this environment
  unsigned n_thd;
  /// thread states
  qthd_info_t *thd_info;
  // # ripple thd blocks associated with this underlying runtime
  unsigned n_blocks;
  /// @brief  A link to the Ripple runtime associated with this one
  ripple_thd_block_t ripple_rt;
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

#ifdef __cplusplus
}
#endif

#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))

// _________________________ Thread spawning ___________________________________
//
//

#include <hexagon_protos.h>

/// A wrapper function that acquires an HVX resource before calling
// the intended function.
RIPPLE_THD_SCALAR
static inline void _qthd_init_hvx(void *void_args_with_id) {
  context_f_args_t *args_with_id = (context_f_args_t *)void_args_with_id;
  void *(*func)(void *) = args_with_id->func;
  void *args = args_with_id->args;
  int err = qurt_hvx_lock(QURT_HVX_MODE_128B);
  if (err != QURT_EOK) {
    qurt_printf("FAILED to lock HVX\n");
    return; // Don't call func without HVX
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

// Layout: qthd_runtime_t | qthd_infos | stacks
RIPPLE_THD_SCALAR
static inline qthd_info_t *_thd_info(qthd_runtime_t *rt, unsigned thid) {
  qthd_info_t *infos = ADD_BYTES(rt, sizeof(qthd_runtime_t), qthd_info_t *);
  return &(infos[thid]);
}

/// @brief Starts a QuRT thread on thread @p thid and runs f(args)
/// @param qthd the QuRT thread object on which f(args) will run
/// @param attr qurt_thread attributes (output)
/// @return error code from qurt_thread_create
RIPPLE_THD_SCALAR static inline __attribute__((always_inline)) int
qthd_create_with_id(qurt_thread_t *qthd, void *(*f)(void *), void *args,
                    qthd_runtime_t *rt, unsigned thid) {
  int ret = 5;
  qurt_thread_attr_t attr;
  // Maintain caller thread priority
  // qurt_thread_t const id = qurt_thread_get_id();
  // int const priority = qurt_thread_get_priority(id);

  qthd_info_t *thd_info = _thd_info(rt, thid);
  qurt_thread_attr_init(&attr);
  qurt_thread_attr_set_stack_size(&attr, RIPPLE_THD_QURT_STACK_SIZE);
  qurt_thread_attr_set_stack_addr(&attr, (unsigned long long *)thd_info->stack);
  qurt_thread_attr_set_priority(&attr, QURT_THREAD_ATTR_PRIORITY_DEFAULT);
  qurt_thread_attr_set_name(&attr, thd_info->name);
  qurt_thread_attr_set_detachstate(&attr, QURT_THREAD_ATTR_CREATE_JOINABLE);
#ifdef RIPPLE_THD_QURT_DEBUG
  qurt_printf("Stack size=%u, stack_addr=0x%x, prio=%u, name=%s\n",
              RIPPLE_THD_QURT_STACK_SIZE, thd_info->stack, 100, thd_info->name);
#endif

  thd_info->ctx.func = f;
  thd_info->ctx.args = args;
  ret =
      qurt_thread_create(qthd, &attr, _qthd_init_hvx, (void *)&(thd_info->ctx));
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
/// @param rt a qthd_runtime_t * that points to the underlying runtime object to
/// be passed to the Ripple thread API
#define ripple_thd_call(underlying, func, args)                                \
  {                                                                            \
    qthd_runtime_t *rt = (qthd_runtime_t *)underlying;                         \
    unsigned n_thd = rt->n_thd;                                                \
    int err;                                                                   \
    int spawned[n_thd];                                                        \
    for (unsigned t = 0; t < n_thd; ++t) {                                     \
      err =                                                                    \
          qthd_create_with_id(&(_thd_info(rt, t)->thread), func, args, rt, t); \
      if (err != 0) {                                                          \
        qurt_printf("Error %i spawning thread %u (skipped)\n", err, t);        \
        spawned[t] = 0;                                                        \
      } else {                                                                 \
        spawned[t] = 1;                                                        \
      }                                                                        \
    }                                                                          \
    for (unsigned t = 0; t < n_thd; ++t) {                                     \
      if (spawned[t] == 0)                                                     \
        continue;                                                              \
      qthd_info_t *thd_info = _thd_info(rt, t);                                \
      err = qurt_thread_join(thd_info->thread, &(thd_info->return_status));    \
      if (err != 0)                                                            \
        qurt_printf("Error %i joining thread %u\n", err, t);                   \
    }                                                                          \
  }

#undef ADD_BYTES
