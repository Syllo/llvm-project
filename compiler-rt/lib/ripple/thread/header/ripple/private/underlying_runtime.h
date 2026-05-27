//===------------------------ underlying_runtime.h ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal API to connect underlying runtime with blocks
//
//===----------------------------------------------------------------------===//

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct ripple_thread_block *ripple_thd_block_t;

// ___________ Private interface to and from the underlying runtime_____________

/// Flat thread ID (runtime-specific)
extern size_t _ripple_thd_flat_id(void *underlying);

/// @brief full number of threads available from the runtime.
/// Must be implemented in the client runtime.
extern size_t _ripple_thd_flat_shape(void *underlying);

/// @brief remembers the base address of the current thread runtime
extern ripple_thd_block_t _ripple_init_thd_block(void *underlying,
                                                 unsigned block_id);

/// @brief remembers the base address of the current thread runtime
extern ripple_thd_block_t _ripple_get_thd_block(void *underlying,
                                                unsigned block_id);

// @brief Cleans up this block in memory, and initializes the related state
/// of the underlying runtime.
extern void _ripple_clear_thd_block(void *underlying, unsigned block_id);

/// @brief initializes @p n_blocks Ripple blocks from an underlying runtime
/// @param max_dims maximum # thread dimensions that will be used with
/// this runtime
/// @param flags Ripple thd feature flags
/// (e.g. RIPPLE_THD_USE_BAR, RIPPLE_THD_USE_DYN)
/// @return block #0
extern ripple_thd_block_t _ripple_thd_init(int PE_id, void *underlying,
                                           unsigned n_blocks, unsigned flags,
                                           unsigned max_dims);

#ifdef __cplusplus
} // extern "C"
#endif
