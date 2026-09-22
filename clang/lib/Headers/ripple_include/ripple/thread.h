//===--------- ripple/thread.h - Declare ripple thread helpers ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#ifndef RIPPLE_PARALLEL_STRINGIFY
#define RIPPLE_PARALLEL_STRINGIFY(x) #x
#endif

// ____________________________ Parallel loop annotations ______________________

/// @brief No flags
#define RIPPLE_THD_PAR_NONE 0
/// @brief do not wait for the other threads at the end of the rpple-annotated
/// loop.
#define RIPPLE_THD_PAR_NOWAIT 1
/// @brief Default flag (NOWAIT)
#define RIPPLE_THD_PAR_DFT (RIPPLE_THD_PAR_NOWAIT)

/**
 * @brief A descriptive loop annotation to split a loop across the specified
 * thread dimension(s) work-items. The minimum amount of work to distribute to
 * each thread is specified using Chunk (integer literal or variable).
 * This implements a "block-cyclic" loop distribution.
 * Notes:
 * - to get a cyclic distribution, set Chunk to 1
 * - to get a distribution in which the iterations are
 *   just evenly split across the threads, set Chunk to n_iterations / n_threads
 * - The only supported flag for now is RIPPLE_THD_FLAG_DFT.
 */
#define ripple_thd_parallel(BlockShape, Chunk, Flags, ...)                     \
  _Pragma(RIPPLE_PARALLEL_STRINGIFY(ripple parallel Block(BlockShape) Dims(    \
      __VA_ARGS__) IgnoreNullStmts ThreadChunk(Chunk) Schedule(static)))

/**
 * @brief Same as 'ripple_parallel_thd_chunk' but dynamically dispatched using
 * an iteration server. This has better load-balancing properties but comes with
 * additional runtime overhead.
 * The only supported flag for now is RIPPLE_THD_FLAG_DFT.
 */
#define ripple_thd_parallel_dyn(BlockShape, Chunk, Flags, ...)                 \
  _Pragma(RIPPLE_PARALLEL_STRINGIFY(ripple parallel Block(BlockShape) Dims(    \
      __VA_ARGS__) IgnoreNullStmts ThreadChunk(Chunk) Schedule(dynamic)))

// Delegate to the ripple/thread.h from the runtime
#if __has_include_next(<ripple/thread.h>)

#include_next <ripple/thread.h>

#else

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RIPPLE_THD_BLOCK_IS_DEFINED
typedef struct ripple_thread_block *ripple_thd_block_t;
#endif

// ______________________ Interface with the underlying runtime ________________
//
//

#ifndef RIPPLE_THD_SCALAR
#ifdef __hexagon__
#define RIPPLE_THD_SCALAR __attribute__((target("no-hvx")))
#else
#define RIPPLE_THD_SCALAR
#endif
#endif

/// Whether we intend to use dynamic load balancing (ripple_parallel_dyn)
#define RIPPLE_THD_USE_DYN 1u
/// Whether we intend to use barriers
#define RIPPLE_THD_USE_BAR (1u << 1)
/// Default flags for ripple_thd_init
#define RIPPLE_THD_DFT (RIPPLE_THD_USE_DYN | RIPPLE_THD_USE_BAR)
/// Whether we don't intend to use any runtime helpers (load-balancing,
/// barriers)
#define RIPPLE_THD_NONE 0

/// @brief Initializes @n_blocks ripple_thd_block_t objects
/// from a pointer to the @p underlying runtime object.
/// @param flags enables features that we want to use,
/// e.g RIPPLE_THD_USE_BAR for barriers,
/// and RIPPLE_THD_USE_DYN for dynamic load balancing
/// @param max_dims the maximum number of thread dimensions to use.
/// This impacts the size of the runtime objects.
/// @return an underlying runtime object that can be passed to the Ripple thread
/// API. This can be the same as @p underlying, depending upon the targeted
/// runtime (cf individual Ripple runtime documentations).
RIPPLE_THD_SCALAR
extern void *ripple_thd_init(int PE_id, void *underlying, unsigned n_blocks,
                             unsigned flags, unsigned max_dims);

/// @brief Tears down the Ripple runtime objects associated with the underlying
/// runtime object @p underlying.
RIPPLE_THD_SCALAR
extern void ripple_thd_exit(void *underlying);

// ____________________________ Ripple core threading API ______________________
//
//

/// A constant to represent that a thread block size is
/// "whatever number to complete the block size to the number of threads
/// managed by the underlying runtime"
#define RIPPLE_THD_DYNAMIC 4294967295u

/// @brief gets a multi-dimensional block-of-threads shape from the @p
/// underlying runtime. The shape is expected to match the number of threads
/// given to us Waits for all the threads to call it before declaring the new
/// shape.
/// @param block_id id of the block we want to give a certain shape
/// @param ...shape the block shape
/// @return a runtime descriptor pointer
/// by @p underlying.
/// TODO: In QHPI, add a state that keeps track of the HW-thread shape
//        (vs. slice-shape as here), to understand which dimensions are
//        overprovisioned.
RIPPLE_THD_SCALAR extern ripple_thd_block_t
ripple_thd_set_block_shape(void *underlying, unsigned block_id, unsigned n_dims,
                           ...);

/// @brief Thread count along dimension dim for @p block
// purity: A block size is always the same for a given block and dim.
// It only changes through ripple_set_block_shape, which produces a new block.
RIPPLE_THD_SCALAR extern __attribute__((always_inline, pure)) __SIZE_TYPE__
ripple_thd_get_block_size(ripple_thd_block_t block, unsigned dim);

/// @brief Thread ID in the block along dimension @p dim.
/// @param block the ripple runtime object
/// @param dim dimension along which we want the thread coordinate.
RIPPLE_THD_SCALAR
// purity: thd id is always the same for a given block and dim.
// It only changes through ripple_set_block_shape, which produces a new block.
__attribute__((pure)) extern __SIZE_TYPE__
ripple_thd_id(ripple_thd_block_t block, unsigned dim);

/// @brief whether we are on the main thread (i.e. thread 0) for dimensions @p
/// dims.
/// @param dims a bit mask representing the dimensions we are testing.
RIPPLE_THD_SCALAR
// purity: this is a pure function of ripple_thd_id, itself pure.
__attribute__((always_inline, pure)) extern int
ripple_thd_is_main(ripple_thd_block_t block, unsigned dims);

/// @brief Waits until all threads along dimensions @p dims have reached the
/// barrier.
/// @param block ripple runtime object pointer
/// @param dims block dimensions that need to synchronize.
RIPPLE_THD_SCALAR
extern void ripple_thd_barrier(ripple_thd_block_t block, unsigned dims);

// _______________________ Ripple iteration server API _________________________
//
//

/// @brief An optional iteration.
typedef struct {
  /// @brief iteration value
  __INT32_TYPE__ value;
  /// @brief whether `value` represents a valid iteration
  __INT32_TYPE__ has_value;
} ripple_opt_it;

/// @brief Thread-safe initializer for an iteration server.
/// Initializes the iteration server associated with dimension dim
/// of thread block \p b.
/// All concurrent users of an iteration server must call init() with the same
/// arguments. Initialization only goes through when all the previous threads
/// have exited the previous instance of this iteration server. Once the first
/// thread initializes the new it_serv, the other threads are free to use it
/// (they still have to call init(), though).
RIPPLE_THD_SCALAR
extern void ripple_it_serv_init(ripple_thd_block_t b, unsigned dim,
                                __INT32_TYPE__ start, __INT32_TYPE__ end,
                                __INT32_TYPE__ stride);

/// @brief Marks the exit of this thread from the use of an iteration server,
/// by decrementing the number of threads that have yet to exit.
/// If @p barrier is true, executes a barrier.
/// Otherwise, the next all-thread sync related to this iteration server
// happens the next time we call init() on this iteration server.
/// @param dim the thread block dimension associated with this iteration server
/// @param barrier whether we want the parallel loop served by the iteration
///        server to be followed by a barrier.
RIPPLE_THD_SCALAR
extern void ripple_it_serv_exit(ripple_thd_block_t b, unsigned dim,
                                int barrier);

/// @brief  Returns the next available iteration
///         for the iteration server associated with dimension \p
///         dim.
/// @return an "optional iteration, i.e. a pair {iteration, has_value}"
RIPPLE_THD_SCALAR
extern ripple_opt_it ripple_it_serv_next(ripple_thd_block_t b, unsigned dim);

#ifdef __cplusplus
} // extern "C"
#endif

#endif