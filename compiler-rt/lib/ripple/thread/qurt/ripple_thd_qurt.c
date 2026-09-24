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

// ______________________________ Core API Helpers _____________________________

RIPPLE_THD_SCALAR
inline size_t _ripple_thd_flat_id(void *underlying) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  size_t hw_id = _qurt_ripple_thd_read_id(thd_name);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  return rt->sw_ids[hw_id];
}

RIPPLE_THD_SCALAR
extern void _set_sw_id(void *underlying, size_t sw_id) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  size_t hw_id = _qurt_ripple_thd_read_id(thd_name);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  rt->sw_ids[hw_id] = sw_id;
}

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
  assert(underlying != RTHD_NULL);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  ripple_thd_block_t block =
      (RTHD_RCAST(qthd_runtime_t *, rt))->ripple_rt[block_id];
  assert(block != RTHD_NULL);
  return block;
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) ripple_thd_block_t
_ripple_init_thd_block(void *underlying, unsigned block_id) {
  ripple_thd_block_t block =
      (ripple_thd_block_t)malloc(sizeof(struct ripple_thread_block));
  assert(block != RTHD_NULL);
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  rt->ripple_rt[block_id] = block;
  return block;
}

/* @brief Cleans up this block in memory, and initializes the related state
of the underlying runtime. */
RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_clear_thd_block(void *underlying, unsigned block_id) {
  qthd_runtime_t *rt = (qthd_runtime_t *)underlying;
  free(rt->ripple_rt[block_id]);
  rt->ripple_rt[block_id] = RTHD_NULL;
}

// _________________________ Underlying runtime ________________________________

RIPPLE_THD_SCALAR
qthd_runtime_t *qthd_runtime_init(unsigned n_thd_) {
  qthd_runtime_t *rt = (qthd_runtime_t *)malloc(sizeof(qthd_runtime_t));
  unsigned max_n_thd = qurt_hvx_get_units();
  if (n_thd_ > max_n_thd) {
    qurt_printf("[Note] Capping # qthd threads to %u\n", max_n_thd);
    n_thd_ = max_n_thd;
  }
  rt->n_thd = n_thd_;
  rt->threads = (qurt_thread_t *)malloc(n_thd_ * sizeof(qurt_thread_t));
  rt->attrs = (qurt_thread_attr_t *)malloc(n_thd_ * sizeof(qurt_thread_attr_t));
  rt->ctxs = (context_f_args_t *)malloc(n_thd_ * sizeof(context_f_args_t));
  rt->sw_ids = (size_t *)malloc(n_thd_ * sizeof(size_t));
  rt->stack = (char **)malloc(n_thd_ * sizeof(char *));
  rt->pool = (qthd_worker_pool_t)malloc(sizeof(struct qthd_worker_pool));

  for (unsigned t = 0; t < n_thd_; ++t) {
    // Generate mini-names "0", "1", etc, which we'll convert back to ints
    // to use as thread ids.
    _qurt_ripple_thd_write_id(rt->names + QURT_RIPPLE_MAX_THD_NAME_SIZE * t, t);
    rt->sw_ids[t] = t;
    // stacks must be 8-byte aligned
    rt->stack[t] = (char *)memalign(8, RIPPLE_THD_QURT_STACK_SIZE);
  }
  rt->return_status = (int *)malloc(n_thd_ * sizeof(int));
  for (unsigned i = 0; i < RIPPLE_THD_QURT_MAX_BLOCKS; ++i) {
    rt->ripple_rt[i] = NULL; // Ripple blocks get initialized in ripple_thd_init
  }
  return rt;
}

RIPPLE_THD_SCALAR
extern void qthd_runtime_exit(qthd_runtime_t *rt) {
  unsigned n_thd = rt->n_thd;
  free(rt->threads);
  free(rt->attrs);
  free(rt->ctxs);
  free(rt->sw_ids);
  for (unsigned t = 0; t < n_thd; ++t) {
    free(rt->stack[t]);
  }
  free(rt->stack);
  free(rt->return_status);
  free(rt->pool);
  for (unsigned block_id = 0; block_id < RIPPLE_THD_QURT_MAX_BLOCKS;
       ++block_id) {
    if (rt->ripple_rt[block_id] == NULL)
      continue;
    free(rt->ripple_rt[block_id]);
    rt->ripple_rt[block_id] = NULL;
  }
  free(rt);
}

/// @brief initializes @p n_blocks ripple thd blocks associated with the
/// underlying runtime.
RIPPLE_THD_SCALAR
extern ripple_thd_block_t ripple_thd_init(int PE_id, void *underlying,
                                          unsigned n_blocks, unsigned flags,
                                          unsigned max_dims) {
  assert(n_blocks <= RIPPLE_THD_QURT_MAX_BLOCKS &&
         "QuRT-Ripple supports up to 2 blocks for now");
  qthd_runtime_t *qrt = (qthd_runtime_t *)underlying;
  qrt->n_blocks = n_blocks;
  _ripple_thd_init(PE_id, underlying, n_blocks, flags, max_dims);
  qurt_thread_t const id = qurt_thread_get_id();
  int parent_priority = qurt_thread_get_priority(id);
  qthd_worker_pool_init(qrt->pool, qrt, parent_priority,
                        RIPPLE_THD_QURT_STACK_SIZE);
  return _ripple_get_thd_block(underlying, 0);
}

RIPPLE_THD_SCALAR
extern void ripple_thd_exit(void *underlying) {
  qthd_runtime_t *qrt = (qthd_runtime_t *)underlying;
  qthd_worker_pool_exit(qrt->pool);
  // Deallocate the shared barriers
  _ripple_thd_barriers_dealloc(qrt->ripple_rt[0]->barriers);
  for (unsigned block = 0; block < qrt->n_blocks; ++block) {
    _ripple_clear_thd_block(underlying, block);
  }
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
inline __attribute__((always_inline)) ripple_thd_barrier_t *
_ripple_thd_barriers_alloc(void *underlying, unsigned n_thd,
                           unsigned max_dims) {
  assert(max_dims <= 3 && "QuRT Ripple runtime supports up to 3 thd dims");
  return (ripple_thd_barrier_t *)malloc(RIPPLE_THD_MAX_BARRIER_3D(n_thd) *
                                        sizeof(ripple_thd_barrier_t));
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barriers_dealloc(ripple_thd_barrier_t *base) {
  free(base);
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) void
_ripple_thd_barrier(ripple_thd_barrier_t *base, unsigned idx) {
  qurt_barrier_wait(&(base[idx]));
}
