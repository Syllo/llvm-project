//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//
// Implements the C-linkage scalar primitives hvx_vdeal_<T> / hvx_vshuff_<T>
// declared in HVX_VectorPair.h.
//
// Each function receives a single HVX-vector-sized typed ripple value and a
// element stride (chunk_size, possibly negative or zero). It:
// 1. Extracts the lo and hi halves of the vector pair.
// 2. Applies Q6_W_vdeal_VVR (deal/unzip) or Q6_W_vshuff_VVR (shuffle/zip)
// with the stride expressed in bytes (elements * element_size).
// 3. Returns the result as the same vector type.
//
// The stride passed to the Q6 intrinsic is already in bytes; callers that work
// in element counts (e.g. hvx_pair_chunked_unzip) multiply by sizeof(T) before
// calling here.
//

#include <cstddef>
#include <cstdint>
#include <hexagon_protos.h>
#include <ripple_hvx.h>

#include "lib_func_attrib.h"

extern "C" {

#define _decl_hvx_vdeals(N_ELEM, T, ELEM_SIZE)                                 \
  RIPPLE_INTRIN_INLINE __attribute__((ripple_vector_candidate))                \
      v##N_ELEM##T ripple_pure_hvx_vdeal_##T(                                 \
      v##N_ELEM##T src, size_t chunk_size) {                                   \
    auto left = Q6_V_lo_W(src);                                                \
    auto right = Q6_V_hi_W(src);                                               \
    return Q6_W_vdeal_VVR(right, left, (chunk_size) * ELEM_SIZE);              \
  }                                                                            \
                                                                               \
  RIPPLE_INTRIN_INLINE __attribute__((ripple_vector_candidate))                \
      v##N_ELEM##T ripple_pure_hvx_vshuff_##T(                                \
      v##N_ELEM##T src, size_t chunk_size) {                                   \
    auto left = Q6_V_lo_W(src);                                                \
    auto right = Q6_V_hi_W(src);                                               \
    return Q6_W_vshuff_VVR(right, left, (chunk_size) * ELEM_SIZE);             \
  }

_decl_hvx_vdeals(256, i8, 1);
_decl_hvx_vdeals(128, i16, 2);
_decl_hvx_vdeals(64, i32, 4);
_decl_hvx_vdeals(32, i64, 8);

_decl_hvx_vdeals(256, u8, 1);
_decl_hvx_vdeals(128, u16, 2);
_decl_hvx_vdeals(64, u32, 4);
_decl_hvx_vdeals(32, u64, 8);

#if __has_bf16__
_decl_hvx_vdeals(128, bf16, 2);
#endif

#if __has_Float16__
_decl_hvx_vdeals(128, f16, 2);
#endif

_decl_hvx_vdeals(64, f32, 4);
_decl_hvx_vdeals(32, f64, 8);

#undef _decl_hvx_vdeals
} // extern "C"
