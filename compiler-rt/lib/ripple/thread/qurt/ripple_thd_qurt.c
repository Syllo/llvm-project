//====------------------------ ripple_thd_qurt.cpp ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// QuRT-specific Ripple thread implementation
//
//===----------------------------------------------------------------------===//

#include "qurt.h"
#include <assert.h>
#include <qurt_barrier.h>
#include <stdint.h>
#include <stdlib.h>

// Use the common definition for ripple_thd_block
typedef qurt_barrier_t ripple_thd_barrier_t;
#define RIPPLE_THD_BARRIER_IS_DEFINED
#include "ripple/private/ripple_thd_barrier.h"
#include "ripple/private/underlying_runtime.h"
#include "ripple/ripple_thd_block.h"
#include "ripple/ripple_thd_conventions.h"
#include "ripple/ripple_thd_qurt.h"
#include "ripple/ripple_thd_qurt_pool.h"
#include <ripple/thread.h>

#define RIPPLE_THD_QURT_STACK_ALIGN 8

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

#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))
#define ALIGN_TO(addr, align)                                                  \
  (((((size_t)(addr)) + (align)-1) / (align)) * (align))
// ______________________________ Core API Helpers _____________________________

RIPPLE_THD_SCALAR
inline size_t _ripple_thd_flat_id(void *underlying) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  size_t hw_id = _qurt_ripple_thd_read_id(thd_name);
#ifdef RIPPLE_THD_QURT_USE_POOL
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  qthd_info_t *info = &(rt->thd_info[hw_id]);
  return info->sw_id;
#else
  return hw_id;
#endif
}

#ifdef RIPPLE_THD_QURT_USE_POOL
RIPPLE_THD_SCALAR
extern void _set_sw_id(void *underlying, size_t sw_id) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  size_t hw_id = _qurt_ripple_thd_read_id(thd_name);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  qthd_info_t *info = &(rt->thd_info[hw_id]);
  info->sw_id = sw_id;
}
#endif

/* @brief full number of threads available from the runtime.*/
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) size_t
_ripple_thd_flat_shape(void *underlying) {
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  return (size_t)rt->n_thd;
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_block_t
_ripple_get_thd_block(void *underlying, unsigned block_id) {
  assert(underlying != NULL);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  return ADD_BYTES(rt->ripple_rt, sizeof(struct ripple_thread_block) * block_id,
                   ripple_thd_block_t);
}

// _________________________ Underlying runtime ________________________________

RIPPLE_THD_SCALAR
qthd_runtime_t *qthd_runtime_init(unsigned n_thd_) {
  unsigned max_n_thd = ripple_num_hvx();
  if (n_thd_ > max_n_thd) {
    qurt_printf("[Note] Capping # qthd threads to %u\n", max_n_thd);
    n_thd_ = max_n_thd;
  }

  // Layout: qthd_runtime_t | thd_pool | qthd_infos | alignment buffer | stacks
  size_t rt_size = sizeof(qthd_runtime_t) + sizeof(struct qthd_worker_pool) +
                   n_thd_ * sizeof(qthd_info_t);
  // stacks: leave a buffer of 8 bytes after the qthd_infos to align the stacks
  rt_size += RIPPLE_THD_QURT_STACK_ALIGN + n_thd_ * RIPPLE_THD_QURT_STACK_SIZE;
  qthd_runtime_t *rt = (qthd_runtime_t *)qurt_malloc(rt_size);
  rt->n_thd = n_thd_;
  rt->pool = ADD_BYTES(rt, sizeof(qthd_runtime_t), qthd_worker_pool_t);
  rt->thd_info =
      ADD_BYTES(rt->pool, sizeof(struct qthd_worker_pool), qthd_info_t *);
  char *align_buf_start =
      ADD_BYTES(rt->thd_info, n_thd_ * sizeof(qthd_info_t), char *);
  char *stack_start =
      (char *)ALIGN_TO(align_buf_start, RIPPLE_THD_QURT_STACK_ALIGN);

  for (unsigned t = 0; t < n_thd_; ++t) {
    qthd_info_t *info = &(rt->thd_info[t]);
    info->stack =
        ADD_BYTES(stack_start, t * RIPPLE_THD_QURT_STACK_SIZE, char *);
    // Generate mini-names "0", "1", etc, which we'll convert back to ints
    // to use as thread ids.
    _qurt_ripple_thd_write_id(info->name, t);
#ifdef RIPPLE_THD_QURT_USE_POOL
    // Initialize sw_id to hw_id
    info->sw_id = t;
#endif

    // stacks must be 8-byte aligned
  }
  return rt;
}

RIPPLE_THD_SCALAR
extern void qthd_runtime_exit(qthd_runtime_t *rt) { qurt_free(rt); }

// | ripple_thread_blocks | it_servs | barriers |
RIPPLE_THD_SCALAR
extern size_t _size_its(unsigned max_dims, unsigned n_thd, unsigned flags);
RIPPLE_THD_SCALAR
extern size_t _size_bars(unsigned max_dims, unsigned n_thd, unsigned flags);

/// @brief size of a thread block (for other modules)
RIPPLE_THD_SCALAR
extern size_t _size_thd_block() { return sizeof(struct ripple_thread_block); }

/// @brief initializes @p n_blocks ripple thd blocks associated with the
/// underlying runtime.
RIPPLE_THD_SCALAR
extern void *ripple_thd_init(int PE_id, void *underlying, unsigned n_blocks,
                             unsigned flags, unsigned max_dims) {
  assert(n_blocks <= RIPPLE_THD_QURT_MAX_BLOCKS &&
         "QuRT-Ripple supports up to 2 blocks for now");
  qthd_runtime_t *qrt = (qthd_runtime_t *)underlying;
  qrt->n_blocks = n_blocks;
  size_t size_block = _size_thd_block();
  size_t size_its = _size_its(max_dims, qrt->n_thd, flags);
  size_t size_bars = _size_bars(max_dims, qrt->n_thd, flags);
  size_t to_alloc = n_blocks * (size_block + size_its) + size_bars;
  qrt->ripple_rt = (ripple_thd_block_t)qurt_malloc(to_alloc);
  assert(qrt->ripple_rt != NULL &&
         "Not enough heap space for the Ripple runtime");
  _ripple_thd_init(PE_id, underlying, n_blocks, qrt->n_thd, flags, max_dims);
#ifdef RIPPLE_THD_QURT_USE_POOL
  // Give the children thread the same priority as their parent
  qurt_thread_t const id = qurt_thread_get_id();
  int parent_priority = qurt_thread_get_priority(id);
  qthd_worker_pool_init(qrt->pool, qrt, parent_priority,
                        RIPPLE_THD_QURT_STACK_SIZE);
#endif
  return qrt;
}

RIPPLE_THD_SCALAR
extern void ripple_thd_exit(void *underlying) {
  assert(underlying != NULL && "underlying runtime object has to be non-NULL");
  qthd_runtime_t *qrt = (qthd_runtime_t *)underlying;
#ifdef RIPPLE_THD_QURT_USE_POOL
  qthd_worker_pool_exit(qrt->pool);
#endif
  qurt_free(qrt->ripple_rt);
}

// ________________________________ Barrier interface __________________________

/// @brief initializes a barrier
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier_init(ripple_thd_barrier_t *barriers, size_t bar_id,
                         size_t n_thd) {
  qurt_barrier_init(&(barriers[bar_id]), n_thd);
}

/// @brief Destroys a barrier
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier_exit(ripple_thd_barrier_t *barriers, size_t bar_id) {
  qurt_barrier_destroy(&(barriers[bar_id]));
}

RIPPLE_THD_SCALAR
extern size_t _ripple_barrier_size() { return sizeof(ripple_thd_barrier_t); }

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier(ripple_thd_barrier_t *base, unsigned idx) {
  qurt_barrier_wait(&(base[idx]));
}

// _________________________ Thread spawning ___________________________________
//
//

#include <hexagon_protos.h>

#ifdef RIPPLE_THD_QURT_USE_POOL

/// @brief Runs @p func(args) in parallel on the threads associated with
/// this runtime's worker pool.
RIPPLE_THD_SCALAR
void ripple_thd_call(void *underlying, void *(*func)(void *), void *args) {
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  qthd_worker_pool_dispatch(rt->pool, func, args);
}

#else // Legacy thread_create/thread_join mode

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
  qurt_thread_t const id = qurt_thread_get_id();
  int parent_priority = qurt_thread_get_priority(id);

  qthd_info_t *thd_info = _thd_info(rt, thid);
  qurt_thread_attr_init(&attr);
  qurt_thread_attr_set_stack_size(&attr, RIPPLE_THD_QURT_STACK_SIZE);
  qurt_thread_attr_set_stack_addr(&attr, (unsigned long long *)thd_info->stack);
  qurt_thread_attr_set_priority(&attr, parent_priority);
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

/// @brief Helper macro that initiates a fork-join, multi-threaded execution
/// of a function with a given set of arguments.
/// @param rt a qthd_runtime_t * that points to the underlying runtime object
/// to be passed to the Ripple thread API
RIPPLE_THD_SCALAR
void ripple_thd_call(void *underlying, void *(*func)(void *), void *args) {
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  unsigned n_thd = rt->n_thd;
  int err;
  int spawned[n_thd];
  for (unsigned t = 0; t < n_thd; ++t) {
    err = qthd_create_with_id(&(_thd_info(rt, t)->thread), func, args, rt, t);
    if (err != 0) {
      qurt_printf("Error %i spawning thread %u (skipped)\n", err, t);
      spawned[t] = 0;
    } else {
      spawned[t] = 1;
    }
  }
  for (unsigned t = 0; t < n_thd; ++t) {
    if (spawned[t] == 0)
      continue;
    qthd_info_t *thd_info = _thd_info(rt, t);
    err = qurt_thread_join(thd_info->thread, &(thd_info->return_status));
    if (err != 0)
      qurt_printf("Error %i joining thread %u\n", err, t);
  }
}
#endif
#undef RIPPLE_THD_QURT_STACK_ALIGN