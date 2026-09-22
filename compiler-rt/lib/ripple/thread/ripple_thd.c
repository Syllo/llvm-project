//====------------------------ ripple_thd_it_serv.c -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Global functions in the ripple runtime
//
//===----------------------------------------------------------------------===//

#include "ripple/private/ripple_thd_barrier.h"
#include "ripple/private/underlying_runtime.h"
#include "ripple/ripple_thd_block.h"
#include "ripple/ripple_thd_conventions.h"
#include <assert.h>
#include <ripple/thread.h>
#include <stdatomic.h>

#include "qurt.h"

// Make sure these symbols get exported (they are inline)
#ifdef __cplusplus
extern "C" {
#endif
extern void ripple_thd_barrier(ripple_thd_block_t block, unsigned dims);
extern int ripple_thd_is_main(ripple_thd_block_t block, unsigned dims);
#ifdef __cplusplus
}
#endif

#ifdef RIPPLE_THD_QURT_DEBUG
#define ripple_dbg(x, ...) qurt_printf((x), __VA_ARGS__)
#else
#define ripple_dbg(x, ...)
#endif

/// @brief initializes barrier objects with the appropriate
// amount of participants.
RIPPLE_THD_SCALAR
__attribute__((always_inline)) inline void
_initialize_barriers(ripple_thd_block_t block, unsigned n_dims,
                     unsigned *shape) {
  unsigned cur_start = 0;
  unsigned f_max = (1 << n_dims);
  block->bar_ends[0] = 0; // barrier(b,0) = no sync
  for (unsigned f = 1; f < f_max; ++f) {
    unsigned n_bar = 1; // # barriers needed for this combination of dimensions
    unsigned n_participants = 1; // # participants in each barrier
    for (unsigned n = 0; n < n_dims; ++n) {
      if (((f >> n) & 1u) == 0)
        n_bar *= shape[n];
      else
        n_participants *= shape[n];
    }
    block->bar_ends[f] = cur_start + n_bar;
    // Note: we are only using barrier_inits
    // that _don't_ allocate anything on the heap.
    // Hence we don't need to destroy barriers here
    for (unsigned b_idx = cur_start; b_idx < block->bar_ends[f]; ++b_idx) {
      _ripple_thd_barrier_init(block->barriers, b_idx, n_participants);
    }
    cur_start += n_bar;
  }
}

/// @brief declares a multi-dimensional block-of-threads shape.
/// Waits for all the threads to call it before declaring the new shape.
/// @param  underlying a pointer to the underlying runtime object
/// @param block_id ID of the block we want to use
/// (when multi-block is supported, otherwise always 0)
/// @param number of dimensions of the block ('s shape)
/// @param ...shape the block shape
/// @return a runtime descriptor pointer
// inline __attribute__((always_inline))
/// @brief Gets the Ripple block from the underlying runtime and
/// gives it a specific shape.
/// The shape is expected to match the number of threads given to us
/// by @p underlying.
/// TODO: In QHPI, add a state that keeps track of the HW-thread shape
//        (vs. slice-shape as here), to understand which dimensions are
//        overprovisioned.
extern RIPPLE_THD_SCALAR ripple_thd_block_t ripple_thd_set_block_shape(
    void *underlying, unsigned block_id, unsigned n_dims, ...) {
  va_list args;
  assert(underlying != RTHD_NULL);
  assert(n_dims > 0 && n_dims <= RIPPLE_MAX_THD_BLOCK_DIMS &&
         "Incorrect n_dims passed to set_block_shape");
  ripple_thd_block_t block = _ripple_get_thd_block(underlying, block_id);
  size_t flat_shape = _ripple_thd_flat_shape(underlying);
  // Wait for all initializations of the previous phase to be done before
  // exiting it.
  while (__atomic_load_n(&(block->to_init), memory_order_acquire) != 0)
    ;
  ripple_dbg("all previous thds have init'd\n");
  // signal that we are exiting the previous phase
  unsigned prev_to_exit =
      __atomic_fetch_sub(&(block->to_exit), 1, memory_order_release);
  // First to exit
  if (prev_to_exit == flat_shape) {
    ripple_dbg("flat shape=%u\n", flat_shape);
    ripple_dbg("block %p: id=%u, underlying=%p, flags=%u, to_exit=%u, "
               "to_init=%u, n_dims=%u, its=%p, barriers=%p\n",
               block, block->block_id, block->underlying, block->flags,
               block->to_exit, block->to_init, block->n_dims, block->its,
               block->barriers);
    // - Prepare a local copy of the next shape
    size_t new_shape[RIPPLE_MAX_THD_BLOCK_DIMS];
    int unk_size_id = -1;
    int shape_changed = block->n_dims != n_dims;
    va_start(args, n_dims);
    if (n_dims == 1) {
      size_t sole_size = va_arg(args, size_t);
      new_shape[0] = sole_size == RIPPLE_THD_DYNAMIC ? flat_shape : sole_size;
      shape_changed |= (new_shape[0] != block->shape[0]);
    } else {
      unsigned i = 0;
      int flat_size = 1;
      for (; i < n_dims; ++i) {
        size_t cur_size = va_arg(args, size_t);
        if (cur_size == RIPPLE_THD_DYNAMIC) {
          assert(unk_size_id == -1);
          unk_size_id = i;
        } else {
          new_shape[i] = cur_size;
          flat_size *= cur_size;
          shape_changed |= (new_shape[i] != block->shape[i]);
        }
      }
      // If there is a dynamic size, calculate it
      unsigned full_flat_size = flat_shape;
      if (unk_size_id != -1) {
        size_t missing_size = full_flat_size / flat_size;
        // The flat block shape size must accomodate the new shape exactly
        assert(flat_size * missing_size == full_flat_size);
        if (new_shape[unk_size_id] != missing_size)
          new_shape[unk_size_id] = missing_size;
      } else {
        assert(flat_size == full_flat_size &&
               "Thread block shape doesn't match number of threads in the "
               "runtime"); // An exact match also in this case
      }
    }
    va_end(args);
    // - Wait for the previous phase to be exited by all the threads
    //   before updating the shape.
    while (__atomic_load_n(&(block->to_exit), memory_order_acquire) > 0)
      ;
    ripple_dbg("1st thd: all thds have exited\n");
    if (shape_changed) {
      ripple_dbg("shape changed! [%u, %u, %u]\n", new_shape[0], new_shape[1],
                 new_shape[2]);
      __atomic_store_n(&(block->n_dims), n_dims, memory_order_relaxed);
      for (unsigned i = 0; i < n_dims; ++i) {
        __atomic_store_n(&(block->shape[i]), new_shape[i],
                         memory_order_relaxed);
      }
      if (block->flags & RIPPLE_THD_USE_BAR) {
        _initialize_barriers(block, n_dims, new_shape);
        __atomic_thread_fence(memory_order_release);
      }
    }
    __atomic_store_n(&(block->to_init), flat_shape - 1, memory_order_release);
    // reinitialize to_exit (which we know is 0) and to_init for the next
    // phase.
    __atomic_store_n(&(block->to_exit), flat_shape, memory_order_release);
    ripple_dbg("to_exit -> %u\n", flat_shape);
    // Also allow other threads to complete the initialization (this function)

  } else {
    // Wait for all threads to have exited the previous phase
    // (by calling set_block_shape) and the first thread to have initialized the
    // shared data (the shape)
    while (__atomic_load_n(&(block->to_exit), memory_order_acquire) !=
           flat_shape)
      ;
    ripple_dbg("other thd: exit step passed, ready to init\n");
    // Mark that we're done with initialization
    __atomic_fetch_sub(&(block->to_init), 1, memory_order_acq_rel);
  }
  return block;
}

RIPPLE_THD_SCALAR extern __attribute__((pure)) size_t
ripple_thd_get_block_size(ripple_thd_block_t block, unsigned dim) {
  return block->shape[dim];
}

/// @brief Access to multi-dimensional Ripple ID from its flat definition.
// purity: see thread.h
RIPPLE_THD_SCALAR
__attribute__((pure)) extern size_t ripple_thd_id(ripple_thd_block_t thdb,
                                                  unsigned dim) {
  size_t flat_id = _ripple_thd_flat_id(thdb->underlying);
  assert(dim < thdb->n_dims);
  if (thdb->n_dims == 1) {
    return flat_id;
  }
  size_t cur_id = flat_id;
  size_t full_id = flat_id;
  // OPT: we could cache intermediary factors here and make this a shorter
  // calculation
  for (int i = 0; i <= dim; ++i) {
    size_t cur_shape = thdb->shape[i];
    cur_id = full_id % cur_shape;
    full_id /= cur_shape;
  }
  return cur_id;
}

RIPPLE_THD_SCALAR
// purity: this is a pure function of ripple_thd_id, itself pure.
__attribute__((always_inline, pure)) extern int
ripple_thd_is_main(ripple_thd_block_t block, unsigned dims) {
  int is_main = 1;
  for (unsigned d = 0, n = block->n_dims; d < n; ++d) {
    is_main &= (((dims >> d) & 1) == 0) || (ripple_thd_id(block, d) == 0);
  }
  return is_main;
}

RIPPLE_THD_SCALAR
extern void ripple_thd_barrier(ripple_thd_block_t block, unsigned dims) {
  assert((block->flags & RIPPLE_THD_USE_BAR) &&
         "Use of barrier while disabled. Use the RIPPLE_THD_USE_BAR flag");
  unsigned clean_dims = (dims & ((1 << block->n_dims) - 1));
  if (clean_dims == 0)
    return;
  // Look into the set of barriers for this combo of dimensions (dims)
  // Our start is the end of our predecessor combo
  unsigned idx = block->bar_ends[clean_dims - 1];
  unsigned sz = 1;
  // Each combination of dimensions has its own set of barriers.
  for (unsigned dim = 0; dim < block->n_dims; ++dim) {
    if (((clean_dims >> dim) & 1u) == 0) {
      idx += ripple_thd_id(block, dim) * sz;
      sz *= block->shape[dim];
    }
  }
  _ripple_thd_barrier(block->barriers, idx);
}

/// First initialization of a block's iteration servers.
/// Supports dynamic scheduling as dimension 0, and one it_serv id per block
RIPPLE_THD_SCALAR
static __attribute__((always_inline)) void
_ripple_serv_init(ripple_thd_block_t b, size_t n_thd, unsigned max_dims) {
  // We need outer_shape() it_servs (i.e. the flat shape without dim 0).
  // This is at most the number of threads divided by 2
  unsigned n = max_dims == 1 ? 1 : n_thd / 2;
  for (unsigned i = 0; i < n; ++i) {
    it_serv_t *serv = &(b->its[i]);
    serv->cur = 0;
    serv->end = 0;
    serv->stride = 1;
    __atomic_store_n(&(serv->to_exit), 0, memory_order_release);
    __atomic_store_n(&(serv->to_init), 0, memory_order_release);
    serv->epoch = 0;
  }
}

RIPPLE_THD_SCALAR
inline __attribute__((always_inline)) size_t
n_bars_to_alloc(unsigned max_dims, unsigned n_slices) {
  assert(max_dims <= 3 && "Max supported thread dimensions: 3\n");
  switch (max_dims) {
  case 1:
    return 1;
  case 2:
    return RIPPLE_THD_MAX_BARRIER_2D(n_slices);
  case 3:
    return RIPPLE_THD_MAX_BARRIER_3D(n_slices);
  }
  return 0;
}

RIPPLE_THD_SCALAR
extern size_t _ripple_barrier_size();

#define ADD_BYTES(addr, bytes, type) (type)(((char *)(addr)) + (bytes))

/// @brief size of a thread block (for other modules)
extern size_t _size_thd_block();

/// @brief size of a set of iteration servers associated with a block.
extern size_t _size_its(unsigned max_dims, unsigned n_thd, unsigned flags) {
  return ((flags & RIPPLE_THD_USE_DYN) == 0) ? 0
         : max_dims == 1                     ? sizeof(it_serv_t)
                         : sizeof(it_serv_t) * (n_thd + 1) / 2;
}

/// @brief size of a set of barriers associated with a block
extern size_t _size_bars(unsigned max_dims, unsigned n_thd, unsigned flags) {
  return ((flags & RIPPLE_THD_USE_BAR) == 0)
             ? 0
             : _ripple_barrier_size() * n_bars_to_alloc(max_dims, n_thd);
}

/// @brief initializes @p n_blocks Ripple blocks from an underlying runtime,
/// assuming the following allocation (starting from thread blocks):
/// | thread_blocks | iteration servers | barriers.
/// Also initializes the iteration servers (barriers are initialized in
/// ripple_thd_set_block_size)
/// @param max_dims maximum # thread dimensions that will be used with
/// this runtime
/// @return block #0
RIPPLE_THD_SCALAR
extern ripple_thd_block_t _ripple_thd_init(int PE_id, void *underlying,
                                           unsigned n_blocks, unsigned n_thd,
                                           unsigned flags, unsigned max_dims) {
  // This version only supports one thd block.
  // TODO: create a loop over block ids
  assert(n_blocks == 1 && "Only one ripple_thd_block is supported for now");
  ripple_thd_block_t block0 = NULL;
  it_serv_t *its_start =
      (it_serv_t *)_ripple_get_thd_block(underlying, n_blocks);
  size_t size_its = _size_its(max_dims, n_thd, flags);
  ripple_thd_barrier_t *bar_start =
      ADD_BYTES(its_start, n_blocks * size_its, ripple_thd_barrier_t *);
  for (unsigned block_id = 0; block_id < n_blocks; ++block_id) {
    ripple_thd_block_t block = _ripple_get_thd_block(underlying, block_id);
    block->underlying = underlying;
    block->block_id = block_id;
    block->flags = flags;
    block->to_exit = n_thd;
    block->to_init = 0;
    block->n_dims = 0; // will be set in ripple_set_block_shape
    if (flags & RIPPLE_THD_USE_DYN) {
      block->its = ADD_BYTES(its_start, block_id * size_its, it_serv_t *);
      _ripple_serv_init(block, n_thd, max_dims);
    }
    if (flags & RIPPLE_THD_USE_BAR) {
      block->barriers = bar_start;
    }
    // Note: Barriers can only be initialized when we know the shape,
    // i.e. in set_block_shape()
    ripple_dbg("block %p: id=%u, underlying=%p, flags=%u, to_exit=%u, "
               "to_init=%u, n_dims=%u, its=%p, barriers=%p\n",
               block, block->block_id, block->underlying, block->flags,
               block->to_exit, block->to_init, block->n_dims, block->its,
               block->barriers);

    if (block_id == 0) {
      block0 = block;
    }
  }
  return block0;
}

/// @brief A block's underlying runtime
RIPPLE_THD_SCALAR
extern void *_ripple_thd_underlying(ripple_thd_block_t block) {
  return block->underlying;
}

#undef ripple_dbg