//===---------------------------- HVX_Rotate.h ------------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//
// Header to support the HVX rotate instructions.
//
//===------------------------------------------------------------------------===

#pragma once
#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple_hvx.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

#define _decl_hvx_rotate(ct, t) ct hvx_rotate_to_lower_##t(ct x, int32_t r)

_decl_hvx_rotate(int8_t, i8);
_decl_hvx_rotate(uint8_t, u8);
_decl_hvx_rotate(int16_t, i16);
_decl_hvx_rotate(uint16_t, u16);
#if __has_bf16__
_decl_hvx_rotate(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_rotate(_Float16, f16);
#endif
_decl_hvx_rotate(int32_t, i32);
_decl_hvx_rotate(uint32_t, u32);
_decl_hvx_rotate(float, f32);
_decl_hvx_rotate(int64_t, i64);
_decl_hvx_rotate(uint64_t, u64);
_decl_hvx_rotate(double, f64);

#undef _decl_hvx_rotate

#ifdef __cplusplus
} // extern "C"
#endif

// Vector candidates — static inline, found by the Ripple pass.
#define _def_ripple_rotate(N, T, C_T, shft)                                    \
  RIPPLE_CANDIDATE_INLINE v##N##T ripple_pure_hvx_rotate_to_lower_##T(         \
      v##N##T in, int32_t n) {                                                 \
    int32_t shift = n << shft;                                                 \
    C_T dummy = 0;                                                             \
    return hvx_cast_from_i32(Q6_V_vror_VR(hvx_cast_to_i32(in), shift), dummy); \
  }

// clang-format off
_def_ripple_rotate(128, i8, int8_t, 0)
_def_ripple_rotate(128, u8, uint8_t, 0)
_def_ripple_rotate(64, i16, int16_t, 1)
_def_ripple_rotate(64, u16, uint16_t, 1)
#if __has_bf16__
_def_ripple_rotate(64, bf16, __bf16, 1)
#endif
#if __has_Float16__
_def_ripple_rotate(64, f16, _Float16, 1)
#endif
_def_ripple_rotate(32, i32, int32_t, 2)
_def_ripple_rotate(32, u32, uint32_t, 2)
_def_ripple_rotate(32, f32, float, 2)
_def_ripple_rotate(16, i64, int64_t, 3)
_def_ripple_rotate(16, u64, uint64_t, 3)
_def_ripple_rotate(16, f64, double, 3)

#undef _def_ripple_rotate
// clang-format on

// Polymorphic entry points (C _Generic / C++ overloads).

#ifndef __cplusplus

#if __has_bf16__
#if __has_Float16__
#define hvx_rotate_to_lower(x, n)                                              \
  _Generic((x),                                                                \
      int8_t: hvx_rotate_to_lower_i8((x), (n)),                                \
      uint8_t: hvx_rotate_to_lower_u8((x), (n)),                               \
      int16_t: hvx_rotate_to_lower_i16((x), (n)),                              \
      uint16_t: hvx_rotate_to_lower_u16((x), (n)),                             \
      __bf16: hvx_rotate_to_lower_bf16((x), (n)),                              \
      _Float16: hvx_rotate_to_lower_f16((x), (n)),                             \
      int32_t: hvx_rotate_to_lower_i32((x), (n)),                              \
      uint32_t: hvx_rotate_to_lower_u32((x), (n)),                             \
      float: hvx_rotate_to_lower_f32((x), (n)),                                \
      int64_t: hvx_rotate_to_lower_i64((x), (n)),                              \
      uint64_t: hvx_rotate_to_lower_u64((x), (n)),                             \
      double: hvx_rotate_to_lower_f64((x), (n)))
#else
#define hvx_rotate_to_lower(x, n)                                              \
  _Generic((x),                                                                \
      int8_t: hvx_rotate_to_lower_i8((x), (n)),                                \
      uint8_t: hvx_rotate_to_lower_u8((x), (n)),                               \
      int16_t: hvx_rotate_to_lower_i16((x), (n)),                              \
      uint16_t: hvx_rotate_to_lower_u16((x), (n)),                             \
      __bf16: hvx_rotate_to_lower_bf16((x), (n)),                              \
      int32_t: hvx_rotate_to_lower_i32((x), (n)),                              \
      uint32_t: hvx_rotate_to_lower_u32((x), (n)),                             \
      float: hvx_rotate_to_lower_f32((x), (n)),                                \
      int64_t: hvx_rotate_to_lower_i64((x), (n)),                              \
      uint64_t: hvx_rotate_to_lower_u64((x), (n)),                             \
      double: hvx_rotate_to_lower_f64((x), (n)))
#endif
#else
#if __has_Float16__
#define hvx_rotate_to_lower(x, n)                                              \
  _Generic((x),                                                                \
      int8_t: hvx_rotate_to_lower_i8((x), (n)),                                \
      uint8_t: hvx_rotate_to_lower_u8((x), (n)),                               \
      int16_t: hvx_rotate_to_lower_i16((x), (n)),                              \
      uint16_t: hvx_rotate_to_lower_u16((x), (n)),                             \
      _Float16: hvx_rotate_to_lower_f16((x), (n)),                             \
      int32_t: hvx_rotate_to_lower_i32((x), (n)),                              \
      uint32_t: hvx_rotate_to_lower_u32((x), (n)),                             \
      float: hvx_rotate_to_lower_f32((x), (n)),                                \
      int64_t: hvx_rotate_to_lower_i64((x), (n)),                              \
      uint64_t: hvx_rotate_to_lower_u64((x), (n)),                             \
      double: hvx_rotate_to_lower_f64((x), (n)))
#else
#define hvx_rotate_to_lower(x, n)                                              \
  _Generic((x),                                                                \
      int8_t: hvx_rotate_to_lower_i8((x), (n)),                                \
      uint8_t: hvx_rotate_to_lower_u8((x), (n)),                               \
      int16_t: hvx_rotate_to_lower_i16((x), (n)),                              \
      uint16_t: hvx_rotate_to_lower_u16((x), (n)),                             \
      int32_t: hvx_rotate_to_lower_i32((x), (n)),                              \
      uint32_t: hvx_rotate_to_lower_u32((x), (n)),                             \
      float: hvx_rotate_to_lower_f32((x), (n)),                                \
      int64_t: hvx_rotate_to_lower_i64((x), (n)),                              \
      uint64_t: hvx_rotate_to_lower_u64((x), (n)),                             \
      double: hvx_rotate_to_lower_f64((x), (n)))
#endif
#endif

#else // __cplusplus

#define _decl_hvx_rotate_spec(ct, t)                                           \
  [[gnu::always_inline, gnu::unused]] static ct hvx_rotate_to_lower(           \
      ct x, int32_t n) {                                                       \
    return hvx_rotate_to_lower_##t(x, n);                                      \
  }

    // clang-format off
_decl_hvx_rotate_spec(int8_t, i8)
_decl_hvx_rotate_spec(uint8_t, u8)
_decl_hvx_rotate_spec(int16_t, i16)
_decl_hvx_rotate_spec(uint16_t, u16)
_decl_hvx_rotate_spec(int32_t, i32)
_decl_hvx_rotate_spec(uint32_t, u32)
_decl_hvx_rotate_spec(int64_t, i64)
_decl_hvx_rotate_spec(uint64_t, u64)

_decl_hvx_rotate_spec(float, f32)
_decl_hvx_rotate_spec(double, f64)
#if __has_bf16__
_decl_hvx_rotate_spec(__bf16, bf16)
#endif
#if __has_Float16__
_decl_hvx_rotate_spec(_Float16, f16)
#endif

#undef _decl_hvx_rotate_spec
// clang-format on

#endif // __cplusplus
