//====-------------------------- __ripple_thd.h ---------------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations common to multiple targets for ripple_thd.h
//
//===----------------------------------------------------------------------===//

#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/// @brief API for the Ripple thread runtime

// ______________________________ Conventions __________________________________

// A constant to represent that a thread block size is dynamic
#define RIPPLE_THD_DYNAMIC SIZE_MAX

/// We think we only need two iteration servers to support
/// 1-d thread parallelization
/// (one for the full-chunk loop and one for the epilogue)
#define RIPPLE_IT_SERV_COUNT 2

/// @brief max number of PE IDs that can correspond to threads + 1.
/// Note that this is a local limitation.
/// The Ripple compiler may define a higher one, but this is all we need for now
/// for the targets we support.
#define RIPPLE_NEXT_BASE_ID 36
#define RIPPLE_THD_BASE_ID 32
#define RIPPLE_THD RIPPLE_THD_BASE_ID
/// Maximum number of block dimensions supported by the Ripple compiler
#define RIPPLE_MAX_BLOCK_DIMS 10

// _____________________________________________________________________________

#include "ripple/ripple_thd_barrier.h"
#include "ripple/ripple_thd_it_serv.h"

#ifdef __cplusplus
#define RTHD_NULL nullptr
extern "C" {
#else
#define RTHD_NULL NULL
#endif

/// Defines the opaque struct from __ripple_vec.h --> ripple_thd_block_t
struct ripple_thread_block {
  /// Backlink to the underlying runtime (optional)
  void *underlying;
  ripple_thd_barrier_t barrier;
  size_t shape[RIPPLE_MAX_BLOCK_DIMS];
  // shared iteration server state
  it_serv_t its[RIPPLE_IT_SERV_COUNT];
};

typedef struct ripple_thread_block *ripple_thd_block_t;

// ___________ Private interface to and from the underlying runtime_____________

/// @brief remembers the base address of the current thread runtime
inline void _ripple_set_thd_block(void *underlying, ripple_thd_block_t block);

/// @brief remembers the base address of the current thread runtime
inline ripple_thd_block_t _ripple_get_thd_block(void *underlying);

// @brief CLeans up this block in memory, and initializes the related state
/// of the underlying runtime.
inline void _ripple_clear_thd_block(ripple_thd_block_t block);

/// @brief full number of threads available from the runtime.
/// Must be implemented in the client runtime.
inline size_t _ripple_flat_shape_thd(ripple_thd_block_t block);

// ______________________ Interface with the underlying runtime ________________
//
//

inline void _ripple_serv_init(ripple_thd_block_t b, unsigned serv_id,
                              unsigned n_thd) {
  it_serv_t *serv = &(b->its[serv_id]);
  serv->cur = 0;
  serv->end = 0;
  serv->stride = 1;
  serv->n_pre_exits = 0;
  serv->mode = 0;
  serv->epoch = 0;
  for (unsigned i = 0; i < n_thd; ++i) {
    serv->local_epoch[i] = 0;
  }
}

/// @brief initializes the Ripple threading runtime from an underlying runtime
// inline __attribute__((always_inline))
inline ripple_thd_block_t ripple_thd_init(int PE_id, void *underlying) {
  ripple_thd_block_t block = _ripple_get_thd_block(underlying);
  block->underlying = underlying;
  size_t n_thd = _ripple_flat_shape_thd(block);
  _ripple_thd_barrier_init(&block->barrier, n_thd);
  for (unsigned s = 0; s < RIPPLE_IT_SERV_COUNT; ++s) {
    _ripple_serv_init(block, s, n_thd);
  }
  _ripple_set_thd_block(underlying, block);
  return block;
}

// inline __attribute__((always_inline))
inline void ripple_thd_exit(ripple_thd_block_t block) {
  // Clean ourselves up if we created ourselves
  _ripple_clear_thd_block(block);
}

// ____________________________ Ripple core threading API  _____________________
//
//

// Note: this interface is currently limited to 1-d blocks of threads.
// It may be extended later if necessary.
// Always use 0 for dim / dims.

/// @brief declares a multi-dimensional block-of-threads shape.
/// @param PE_id the processing element ID. For HVX, use the HVX_THD constant
/// @param ...shape the block shape
/// @return a runtime descriptor pointer
// inline __attribute__((always_inline))
/// @brief Gets the Ripple block from the underlying runtime and
/// gives it a specific shape.
/// The shape is expected to match the number of threads given to us
/// by @p underlying.
inline ripple_thd_block_t ripple_thd_set_block_shape(void *underlying,
                                                     unsigned n_dims, ...) {
  va_list args;
  assert(n_dims == 1);
  ripple_thd_block_t block = _ripple_get_thd_block(underlying);
  int unk_size_id = -1;
  va_start(args, n_dims);
  if (n_dims == 1) {
    size_t sole_size = va_arg(args, size_t);
    block->shape[0] = sole_size == RIPPLE_THD_DYNAMIC
                          ? _ripple_flat_shape_thd(block)
                          : sole_size;
  } else {
    unsigned i = 0;
    int flat_size = 1;
    for (; i < n_dims; ++i) {
      size_t cur_size = va_arg(args, size_t);
      if (cur_size == RIPPLE_THD_DYNAMIC) {
        assert(unk_size_id == -1);
        unk_size_id = i;
      } else {
        block->shape[i] = cur_size;
        flat_size *= cur_size;
      }
    }
    // If there is a dynamic size, calculate it
    unsigned full_flat_size = _ripple_flat_shape_thd(block);
    if (unk_size_id != -1) {
      assert(full_flat_size % flat_size == 0);
      block->shape[unk_size_id] = full_flat_size / flat_size;
    } else {
      assert(flat_size == full_flat_size); // Restricted for now.
    }
  }
  va_end(args);
  return block;
}

/// @brief Waits until all threads have reached the barrier
/// @param block ripple runtime object pointer
/// @param ...dims block dimensions that need to synchronize.
static inline void ripple_thd_barrier(ripple_thd_block_t block, unsigned dims);

#ifdef __cplusplus
} // extern "C"
#endif

/// @brief Thread ID in the block along dimension @p dim.
/// @param block the ripple runtime object
/// @param dim dimension along which we want the thread coordinate.
// inline __attribute__((always_inline))
static inline __attribute__((always_inline)) size_t
ripple_thd_id(ripple_thd_block_t block, unsigned dim);

/// @brief Thread count along dimension dim for @p block
// inline __attribute__((always_inline))
static inline __attribute__((always_inline)) size_t
ripple_thd_get_block_size(ripple_thd_block_t block, unsigned dim) {
  return block->shape[dim];
}

/// @brief whether we are on the main thread.
/// TODO: Only one-dimensional case supported for now
// inline __attribute__((always_inline))
static inline int ripple_thd_is_main(ripple_thd_block_t block, unsigned dims) {
  return ripple_thd_id(block, 0) == 0;
}

// _________ pthread/qthread underlying runtime management codegen _____________

#define decl_thread_methods(rt_prefix)                                         \
  /* @brief full number of threads available from the runtime.*/               \
  inline __attribute((always_inline)) size_t _ripple_flat_shape_thd(           \
      ripple_thd_block_t block) {                                              \
    rt_prefix##_runtime_t *rt = (rt_prefix##_runtime_t *)block->underlying;    \
    return (size_t)rt->n_thd;                                                  \
  }                                                                            \
                                                                               \
  inline ripple_thd_block_t _ripple_get_thd_block(void *underlying) {          \
    rt_prefix##_runtime_t *rt = (rt_prefix##_runtime_t *)underlying;           \
    ripple_thd_block_t block = (ripple_thd_block_t)rt->ripple_rt;              \
    if (block == RTHD_NULL) {                                                  \
      block = (ripple_thd_block_t)malloc(sizeof(struct ripple_thread_block));  \
      assert(block != RTHD_NULL);                                              \
      rt->ripple_rt = block;                                                   \
    }                                                                          \
    return block;                                                              \
  }                                                                            \
                                                                               \
  /* @brief remembers the base address of the current thread runtime */        \
  inline void _ripple_set_thd_block(void *underlying,                          \
                                    ripple_thd_block_t block) {                \
    rt_prefix##_runtime_t *rt = (rt_prefix##_runtime_t *)underlying;           \
    rt->ripple_rt = block;                                                     \
  }                                                                            \
                                                                               \
  /* @brief Cleans up this block in memory, and initializes the related state  \
 of the underlying runtime. */                                                 \
  inline void _ripple_clear_thd_block(ripple_thd_block_t block) {              \
    rt_prefix##_runtime_t *rt = (rt_prefix##_runtime_t *)block->underlying;    \
    free(rt->ripple_rt);                                                       \
    rt->ripple_rt = RTHD_NULL;                                                 \
  }
