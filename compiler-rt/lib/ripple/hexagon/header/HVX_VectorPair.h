//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//

#pragma once
#include <ripple.h>
#include <stddef.h>
#include <stdint.h>

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <ripple_hvx.h>

// ripple_vector_candidate definitions.
#define _def_ripple_hvx_vdeals(N_ELEM, T, ELEM_SIZE)                           \
  RIPPLE_CANDIDATE_INLINE v##N_ELEM##T ripple_pure_hvx_vdeal_##T(              \
      v##N_ELEM##T src, size_t chunk_size) {                                   \
    HVX_Vector left = Q6_V_lo_W(src);                                          \
    HVX_Vector right = Q6_V_hi_W(src);                                         \
    return Q6_W_vdeal_VVR(right, left, (chunk_size) * ELEM_SIZE);              \
  }                                                                            \
                                                                               \
  RIPPLE_CANDIDATE_INLINE v##N_ELEM##T ripple_pure_hvx_vshuff_##T(             \
      v##N_ELEM##T src, size_t chunk_size) {                                   \
    HVX_Vector left = Q6_V_lo_W(src);                                          \
    HVX_Vector right = Q6_V_hi_W(src);                                         \
    return Q6_W_vshuff_VVR(right, left, (chunk_size) * ELEM_SIZE);             \
  }

_def_ripple_hvx_vdeals(256, i8, 1);
_def_ripple_hvx_vdeals(128, i16, 2);
_def_ripple_hvx_vdeals(64, i32, 4);
_def_ripple_hvx_vdeals(32, i64, 8);

_def_ripple_hvx_vdeals(256, u8, 1);
_def_ripple_hvx_vdeals(128, u16, 2);
_def_ripple_hvx_vdeals(64, u32, 4);
_def_ripple_hvx_vdeals(32, u64, 8);

#if __has_bf16__
_def_ripple_hvx_vdeals(128, bf16, 2);
#endif

#if __has_Float16__
_def_ripple_hvx_vdeals(128, f16, 2);
#endif

_def_ripple_hvx_vdeals(64, f32, 4);
_def_ripple_hvx_vdeals(32, f64, 8);

#undef _def_ripple_hvx_vdeals

// Scalar declarations. Ripple candidate names mirror these entry points.

#ifdef __cplusplus
extern "C" {
#endif

#define _decl_hvx_vdeals(ct, t)                                                \
  __attribute__((used, always_inline, weak)) extern ct hvx_vdeal_##t(          \
      ct src, size_t chunk_size);                                              \
                                                                               \
  __attribute__((used, always_inline, weak)) extern ct hvx_vshuff_##t(         \
      ct src, size_t chunk_size)

_decl_hvx_vdeals(int8_t, i8);
_decl_hvx_vdeals(int16_t, i16);
_decl_hvx_vdeals(int32_t, i32);
_decl_hvx_vdeals(int64_t, i64);

_decl_hvx_vdeals(uint8_t, u8);
_decl_hvx_vdeals(uint16_t, u16);
_decl_hvx_vdeals(uint32_t, u32);
_decl_hvx_vdeals(uint64_t, u64);

_decl_hvx_vdeals(double, f64);
_decl_hvx_vdeals(float, f32);
#if __has_bf16__
_decl_hvx_vdeals(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_vdeals(_Float16, f16);
#endif

#undef _decl_hvx_vdeals

#ifdef __cplusplus
} // extern "C"
#endif

#ifndef __cplusplus

#define _decl_hvx_vdeal_anyint(CT, Name, T)                                    \
  RIPPLE_INTRIN_INLINE CT __ripple_hvx_vdeal_##Name(CT val,                    \
                                                    size_t chunk_size) {       \
    RIPPLE_STATIC_ASSERT(sizeof(CT) == 1 || sizeof(CT) == 2 ||                 \
                             sizeof(CT) == 4 || sizeof(CT) == 8,               \
                         "unsupported scalar size");                           \
    switch (sizeof(CT)) {                                                      \
    case 1:                                                                    \
      return (CT)hvx_vdeal_##T##8((val), (chunk_size));                        \
    case 2:                                                                    \
      return (CT)hvx_vdeal_##T##16((val), (chunk_size));                       \
    case 4:                                                                    \
      return (CT)hvx_vdeal_##T##32((val), (chunk_size));                       \
    case 8:                                                                    \
      return (CT)hvx_vdeal_##T##64((val), (chunk_size));                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  RIPPLE_INTRIN_INLINE CT __ripple_hvx_vshuff_##Name(CT val,                   \
                                                     size_t chunk_size) {      \
    RIPPLE_STATIC_ASSERT(sizeof(CT) == 1 || sizeof(CT) == 2 ||                 \
                             sizeof(CT) == 4 || sizeof(CT) == 8,               \
                         "unsupported scalar size");                           \
    switch (sizeof(CT)) {                                                      \
    case 1:                                                                    \
      return (CT)hvx_vshuff_##T##8((val), (chunk_size));                       \
    case 2:                                                                    \
      return (CT)hvx_vshuff_##T##16((val), (chunk_size));                      \
    case 4:                                                                    \
      return (CT)hvx_vshuff_##T##32((val), (chunk_size));                      \
    case 8:                                                                    \
      return (CT)hvx_vshuff_##T##64((val), (chunk_size));                      \
    }                                                                          \
  }

_decl_hvx_vdeal_anyint(char, char_i, i);
_decl_hvx_vdeal_anyint(char, char_u, u);
_decl_hvx_vdeal_anyint(signed char, schar, i);
_decl_hvx_vdeal_anyint(unsigned char, uchar, u);
_decl_hvx_vdeal_anyint(signed short, sshort, i);
_decl_hvx_vdeal_anyint(unsigned short, ushort, u);
_decl_hvx_vdeal_anyint(signed int, sint, i);
_decl_hvx_vdeal_anyint(unsigned int, uint, u);
_decl_hvx_vdeal_anyint(signed long, slong, i);
_decl_hvx_vdeal_anyint(unsigned long, ulong, u);
_decl_hvx_vdeal_anyint(signed long long, sllong, i);
_decl_hvx_vdeal_anyint(unsigned long long, ullong, u);

#undef _decl_hvx_vdeal_anyint

#if __has_bf16__
#define hvx_vdeal_extra_bf16(src, chunk_size)                                  \
  , __bf16 : hvx_vdeal_bf16((src), (chunk_size))
#define hvx_vshuff_extra_bf16(src, chunk_size)                                 \
  , __bf16 : hvx_vshuff_bf16((src), (chunk_size))
#else
#define hvx_vdeal_extra_bf16(src, chunk_size)
#define hvx_vshuff_extra_bf16(src, chunk_size)
#endif

#if __has_Float16__
#define hvx_vdeal_extra_f16(src, chunk_size)                                   \
  , _Float16 : hvx_vdeal_f16((src), (chunk_size))
#define hvx_vshuff_extra_f16(src, chunk_size)                                  \
  , _Float16 : hvx_vshuff_f16((src), (chunk_size))
#else
#define hvx_vdeal_extra_f16(src, chunk_size)
#define hvx_vshuff_extra_f16(src, chunk_size)
#endif

#define __hvx_vdeal(src, chunk_size)                                           \
  _Generic((src),                                                              \
      char: __ripple_char_is_signed                                            \
          ? __ripple_hvx_vdeal_char_i((src), (chunk_size))                     \
          : __ripple_hvx_vdeal_char_u((src), (chunk_size)),                    \
      signed char: __ripple_hvx_vdeal_schar((src), (chunk_size)),              \
      unsigned char: __ripple_hvx_vdeal_uchar((src), (chunk_size)),            \
      signed short: __ripple_hvx_vdeal_sshort((src), (chunk_size)),            \
      unsigned short: __ripple_hvx_vdeal_ushort((src), (chunk_size)),          \
      signed int: __ripple_hvx_vdeal_sint((src), (chunk_size)),                \
      unsigned int: __ripple_hvx_vdeal_uint((src), (chunk_size)),              \
      signed long: __ripple_hvx_vdeal_slong((src), (chunk_size)),              \
      unsigned long: __ripple_hvx_vdeal_ulong((src), (chunk_size)),            \
      signed long long: __ripple_hvx_vdeal_sllong((src), (chunk_size)),        \
      unsigned long long: __ripple_hvx_vdeal_ullong((src), (chunk_size)),      \
      float: hvx_vdeal_f32((src), (chunk_size)),                               \
      double: hvx_vdeal_f64((src), (chunk_size))                               \
          hvx_vdeal_extra_f16(src, (chunk_size))                               \
              hvx_vdeal_extra_bf16(src, (chunk_size)))

#define __hvx_vshuff(src, chunk_size)                                          \
  _Generic((src),                                                              \
      char: __ripple_char_is_signed                                            \
          ? __ripple_hvx_vshuff_char_i((src), (chunk_size))                    \
          : __ripple_hvx_vshuff_char_u((src), (chunk_size)),                   \
      signed char: __ripple_hvx_vshuff_schar((src), (chunk_size)),             \
      unsigned char: __ripple_hvx_vshuff_uchar((src), (chunk_size)),           \
      signed short: __ripple_hvx_vshuff_sshort((src), (chunk_size)),           \
      unsigned short: __ripple_hvx_vshuff_ushort((src), (chunk_size)),         \
      signed int: __ripple_hvx_vshuff_sint((src), (chunk_size)),               \
      unsigned int: __ripple_hvx_vshuff_uint((src), (chunk_size)),             \
      signed long: __ripple_hvx_vshuff_slong((src), (chunk_size)),             \
      unsigned long: __ripple_hvx_vshuff_ulong((src), (chunk_size)),           \
      signed long long: __ripple_hvx_vshuff_sllong((src), (chunk_size)),       \
      unsigned long long: __ripple_hvx_vshuff_ullong((src), (chunk_size)),     \
      float: hvx_vshuff_f32((src), (chunk_size)),                              \
      double: hvx_vshuff_f64((src), (chunk_size))                              \
          hvx_vshuff_extra_bf16((src), (chunk_size))                           \
              hvx_vshuff_extra_f16((src), (chunk_size)))

// ________________________ Public C interface _________________________________

#define hvx_pair_chunked_unzip(src, chunk_size)                                \
  ({                                                                           \
    size_t __chunk_size = chunk_size;                                          \
    __hvx_vdeal(src, -__chunk_size);                                           \
  })

#define hvx_pair_chunked_zip(src, chunk_size)                                  \
  ({                                                                           \
    size_t __chunk_size = chunk_size;                                          \
    __hvx_vshuff(src, -__chunk_size);                                          \
  })

#define hvx_pair_chunked_unzip_list(src, chunk_size, list_size)                \
  ({                                                                           \
    size_t __chunk_size = chunk_size;                                          \
    size_t __list_size = list_size;                                            \
    __hvx_vdeal(src, __list_size - __chunk_size);                              \
  })

#define hvx_pair_chunked_zip_list(src, chunk_size, list_size)                  \
  ({                                                                           \
    size_t __chunk_size = chunk_size;                                          \
    size_t __list_size = list_size;                                            \
    __hvx_vshuff(src, __list_size - __chunk_size);                             \
  })

#define hvx_pair_2x2_transpose(src, chunk_size)                                \
  ({                                                                           \
    size_t __chunk_size = chunk_size;                                          \
    __hvx_vshuff(src, __chunk_size);                                           \
  })

#define hvx_pair_2x2_transpose_dec(src, chunk_size)                            \
  __hvx_vdeal(src, (chunk_size))

#define hvx_pair_2x2_transpose_inc(src, chunk_size)                            \
  __hvx_vshuff(src, (chunk_size))

#endif // !__cplusplus

#ifdef __cplusplus

#define _decl_hvx_vdeals_spec_int(CT, UI)                                      \
  [[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static CT __hvx_vdeal(            \
      CT Val, size_t chunk_size) {                                             \
    RIPPLE_STATIC_ASSERT(sizeof(CT) == 1 || sizeof(CT) == 2 ||                 \
                             sizeof(CT) == 4 || sizeof(CT) == 8,               \
                         "unsupported scalar size");                           \
    switch (sizeof(Val)) {                                                     \
    case 1:                                                                    \
      return hvx_vdeal_##UI##8((Val), (chunk_size));                           \
    case 2:                                                                    \
      return hvx_vdeal_##UI##16((Val), (chunk_size));                          \
    case 4:                                                                    \
      return hvx_vdeal_##UI##32((Val), (chunk_size));                          \
    case 8:                                                                    \
      return hvx_vdeal_##UI##64((Val), (chunk_size));                          \
    }                                                                          \
  }                                                                            \
                                                                               \
  [[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static CT __hvx_vshuff(           \
      CT Val, size_t chunk_size) {                                             \
    RIPPLE_STATIC_ASSERT(sizeof(CT) == 1 || sizeof(CT) == 2 ||                 \
                             sizeof(CT) == 4 || sizeof(CT) == 8,               \
                         "unsupported scalar size");                           \
    switch (sizeof(Val)) {                                                     \
    case 1:                                                                    \
      return hvx_vshuff_##UI##8((Val), (chunk_size));                          \
    case 2:                                                                    \
      return hvx_vshuff_##UI##16((Val), (chunk_size));                         \
    case 4:                                                                    \
      return hvx_vshuff_##UI##32((Val), (chunk_size));                         \
    case 8:                                                                    \
      return hvx_vshuff_##UI##64((Val), (chunk_size));                         \
    }                                                                          \
  }

// clang-format off
_decl_hvx_vdeals_spec_int(signed char, i)
_decl_hvx_vdeals_spec_int(unsigned char, u)
_decl_hvx_vdeals_spec_int(short, i)
_decl_hvx_vdeals_spec_int(unsigned short, u)
_decl_hvx_vdeals_spec_int(int, i)
_decl_hvx_vdeals_spec_int(unsigned int, u)
_decl_hvx_vdeals_spec_int(long, i)
_decl_hvx_vdeals_spec_int(unsigned long, u)
_decl_hvx_vdeals_spec_int(long long, i)
_decl_hvx_vdeals_spec_int(unsigned long long, u)
// clang-format on

#undef _decl_hvx_vdeals_spec_int

    // plain char — signedness is implementation-defined
    [[gnu::always_inline]] RIPPLE_MAYBE_UNUSED
    static char __hvx_vdeal(const char Val, size_t chunk_size) {
  return __ripple_char_is_signed ? hvx_vdeal_i8((Val), (chunk_size))
                                 : hvx_vdeal_u8((Val), (chunk_size));
}

[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static char
__hvx_vshuff(const char Val, size_t chunk_size) {
  return __ripple_char_is_signed ? hvx_vshuff_i8((Val), (chunk_size))
                                 : hvx_vshuff_u8((Val), (chunk_size));
}

#if __has_Float16__
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static _Float16
__hvx_vdeal(_Float16 Val, size_t chunk_size) {
  return hvx_vdeal_f16((Val), (chunk_size));
}
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static _Float16
__hvx_vshuff(_Float16 Val, size_t chunk_size) {
  return hvx_vshuff_f16((Val), (chunk_size));
}
#endif

#if __has_bf16__
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static __bf16
__hvx_vdeal(__bf16 Val, size_t chunk_size) {
  return hvx_vdeal_bf16((Val), (chunk_size));
}
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static __bf16
__hvx_vshuff(__bf16 Val, size_t chunk_size) {
  return hvx_vshuff_bf16((Val), (chunk_size));
}
#endif

[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static float
__hvx_vdeal(float Val, size_t chunk_size) {
  return hvx_vdeal_f32((Val), (chunk_size));
}
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static float
__hvx_vshuff(float Val, size_t chunk_size) {
  return hvx_vshuff_f32((Val), (chunk_size));
}

[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static double
__hvx_vdeal(double Val, size_t chunk_size) {
  return hvx_vdeal_f64((Val), (chunk_size));
}
[[gnu::always_inline]] RIPPLE_MAYBE_UNUSED static double
__hvx_vshuff(double Val, size_t chunk_size) {
  return hvx_vshuff_f64((Val), (chunk_size));
}

// ----------------------------------------------------------------------------
// Public C++ template definitions
// ----------------------------------------------------------------------------

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_chunked_unzip(T x, size_t chunk_size) {
  return __hvx_vdeal(x, -chunk_size);
}

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_chunked_zip(T x, size_t chunk_size) {
  return __hvx_vshuff(x, -chunk_size);
}

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_chunked_unzip_list(T x, size_t chunk_size, size_t list_size) {
  return __hvx_vdeal(x, list_size - chunk_size);
}

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_chunked_zip_list(T x, size_t chunk_size, size_t list_size) {
  return __hvx_vshuff(x, list_size - chunk_size);
}

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_2x2_transpose_dec(T x, size_t chunk_size) {
  return __hvx_vdeal(x, chunk_size);
}

template <typename T>
__attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static T
hvx_pair_2x2_transpose_inc(T x, size_t chunk_size) {
  return __hvx_vshuff(x, chunk_size);
}

#endif // __cplusplus
