//====------------------------- ripple_thd_it_serv.h ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Iteration server header (part of the Ripple thd runtime)
//
//===----------------------------------------------------------------------===//

/// @brief C++ class decls for iteration servers.

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO: This static max # threads is not ideal.
#if defined(__hexagon__)
#define RIPPLE_MAX_THD_COUNT 32
#elif defined(__x86_64__)
#define RIPPLE_MAX_THD_COUNT 64
#elif defined(__aarch64__)
#define RIPPLE_MAX_THD_COUNT 64
#endif

/// @brief An iteration server shared state.
typedef struct _ripple_it_serv {
  // Current value of the iterator
  int32_t cur;
  // Exclusive bound (first value out of range).
  // If stride is >=0, this is the loop upper bound, otherwise its lower bound.
  int32_t end;
  int32_t stride;
  int32_t n_pre_exits;
  int32_t mode;
  // global epoch for all iteration servers
  size_t epoch;
  // local epochs
  size_t local_epoch[RIPPLE_MAX_THD_COUNT];

} it_serv_t;

/// @brief An optional iteration.
typedef struct {
  /// @brief iteration value
  int32_t value;
  /// @brief whether `value` represents a valid iteration
  int32_t has_value;
} ripple_opt_it;

typedef struct ripple_thread_block *ripple_thd_block_t;

/// @brief Thread-safe initializer for an iteration server.
/// Initializes the @p serv_id^th iteration server and increments the epoch if
/// the previous use of iteration server @p serv_id is depleted.
/// One of the threads at that point will reset the iteration server.
/// All concurrent users of an iteration server are supposed to use the same
/// initializer.
/// Note: Some calculations may still be running from the previous instance
/// with already-served iterations. This is desired in the no-barrier behavior.
extern void ripple_it_serv_init(ripple_thd_block_t b, unsigned int serv_id,
                                int32_t start, int32_t end, int32_t stride);

/// @brief Marks the exit of this thread from the use of an iteration server,
/// by decrementing the number of participants.
/// If @p barrier is true,
/// In the latter case, atomically resets the iteration server
/// to a trivially depleted one.
/// @param serv_id the index of the iteration server to use.
/// A runtime like this one uses two.
/// @param barrier whether we want the parallel loop served by the iteration
/// server
///        to be followed by a barrier.
extern void ripple_it_serv_exit(ripple_thd_block_t b, unsigned int serv_id,
                                int barrier);

/// @brief  Returns the next available iteration
///         for the @   p serv_id ^th iteration server.
/// @return an "optional iteration, i.e. a pair {iteration, has_value}"
extern ripple_opt_it ripple_it_serv_next(ripple_thd_block_t b,
                                         unsigned int serv_id);

#ifdef __cplusplus
}
#endif
