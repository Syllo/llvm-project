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

#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))
#define ALIGN_TO(addr, align) (((addr) + (align)-1) / (align)) * (align)
// ______________________________ Core API Helpers _____________________________

RIPPLE_THD_SCALAR
inline size_t _ripple_thd_flat_id(void *underlying) {
  char thd_name[QURT_RIPPLE_MAX_THD_NAME_SIZE];
  qurt_thread_get_name(thd_name, QURT_RIPPLE_MAX_THD_NAME_SIZE);
  return _qurt_ripple_thd_read_id(thd_name);
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

  // Layout: qthd_runtime_t | qthd_infos | stacks
  size_t rt_size = sizeof(qthd_runtime_t) + n_thd_ * sizeof(qthd_info_t);
  // stacks: align to 8 and allocate
  size_t before_stack_size = ALIGN_TO(rt_size, 8);
  rt_size = before_stack_size + n_thd_ * RIPPLE_THD_QURT_STACK_SIZE;
  qthd_runtime_t *rt = (qthd_runtime_t *)qurt_malloc(rt_size);
  rt->thd_info = ADD_BYTES(rt, sizeof(qthd_runtime_t), qthd_info_t *);
  rt->n_thd = n_thd_;

  for (unsigned t = 0; t < n_thd_; ++t) {
    qthd_info_t *info = &(rt->thd_info[t]);
    info->stack = ADD_BYTES(
        rt, before_stack_size + t * RIPPLE_THD_QURT_STACK_SIZE, char *);
    // Generate mini-names "0", "1", etc, which we'll convert back to ints
    // to use as thread ids.
    _qurt_ripple_thd_write_id(info->name, t);
    // stacks must be 8-byte aligned
  }
  return rt;
}

RIPPLE_THD_SCALAR
extern void qthd_runtime_exit(qthd_runtime_t *rt) { qurt_free(rt); }

// | ripple_thread_blocks | it_servs | barriers |

extern size_t _size_its(unsigned max_dims, unsigned n_thd, unsigned flags);
extern size_t _size_bars(unsigned max_dims, unsigned n_thd, unsigned flags);

/// @brief size of a thread block (for other modules)
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
  return qrt;
}

RIPPLE_THD_SCALAR
extern void ripple_thd_exit(void *underlying) {
  assert(underlying != NULL && "underlying runtime object has to be non-NULL");
  qthd_runtime_t *qrt = (qthd_runtime_t *)underlying;
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
