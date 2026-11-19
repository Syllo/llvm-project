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
void qurt_thread_exit(int status);

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

typedef struct qthd_worker_pool *qthd_worker_pool_t;

typedef struct qthd_info {
  /// @brief We remember one pthread_t per thread
  qurt_thread_t thread;
  /// Contexts passed to each thread
  context_f_args_t ctx;
  /// @brief an array to store the names (really, the IDs) of the threads
  char name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  /// @brief thread stacks (one per thread)
  char *stack;
  /// Software id.
  size_t sw_id;
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
  // Worker pool for lightweight multi-threaded calls
  qthd_worker_pool_t pool;
  /// @brief  A link to the Ripple runtime associated with this one
  ripple_thd_block_t ripple_rt;
} qthd_runtime_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Creates a qthread-based environment that runs @p n_thd threads,
/// Which can be used to create a Ripple block.
RIPPLE_THD_SCALAR
extern qthd_runtime_t *qthd_runtime_init(unsigned n_thd);

/// @brief Tears down the qthread-based environment
RIPPLE_THD_SCALAR
extern void qthd_runtime_exit(qthd_runtime_t *rt);

/// @brief Executes @p func(args) on all threads associated with @p underlying
RIPPLE_THD_SCALAR
extern void ripple_thd_call(void *underlying, void *(*func)(void *),
                            void *args);

#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))

// Layout: qthd_runtime_t | qthd_infos | stacks
RIPPLE_THD_SCALAR
static inline qthd_info_t *_thd_info(qthd_runtime_t *rt, unsigned thid) {
  return &(rt->thd_info[thid]);
}

#undef ADD_BYTES

#ifdef __cplusplus
}
#endif
