//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//

#pragma once
#include "__ripple_vec.h"
#include <stddef.h>
#include <stdint.h>

#include <ripple_hvx.h>

#define RIPPLE_VECTOR_PAIR_CANDIDATE __attribute__((ripple_vector_candidate))

#ifdef __cplusplus
extern "C" {
#endif

extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v256u8 ripple_pure_hvx_vdeal_u8(v256u8 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v128u16
ripple_pure_hvx_vdeal_u16(v128u16 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64u32
ripple_pure_hvx_vdeal_u32(v64u32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32u64
ripple_pure_hvx_vdeal_u64(v32u64 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v256i8
ripple_pure_hvx_vdeal_i8(v256i8 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v128i16
ripple_pure_hvx_vdeal_i16(v128i16 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64i32
ripple_pure_hvx_vdeal_i32(v64i32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32i64
ripple_pure_hvx_vdeal_i64(v32i64 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64f32
ripple_pure_hvx_vdeal_f32(v64f32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32f64
ripple_pure_hvx_vdeal_f64(v32f64 src, size_t chunk_size);

#if __has_Float16__
extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v128f16 ripple_pure_hvx_vdeal_f16(v128f16 src, size_t chunk_sizze);
#endif

#if __has_bf16__
extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v128bf16 ripple_pure_hvx_vdeal_bf16(v128bf16 src, size_t chunk_size);
#endif

extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v256u8 ripple_pure_hvx_vshuff_u8(v256u8 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v128u16
ripple_pure_hvx_vshuff_u16(v128u16 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64u32
ripple_pure_hvx_vshuff_u32(v64u32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32u64
ripple_pure_hvx_vshuff_u64(v32u64 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v256i8
ripple_pure_hvx_vshuff_i8(v256i8 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v128i16
ripple_pure_hvx_vshuff_i16(v128i16 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64i32
ripple_pure_hvx_vshuff_i32(v64i32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32i64
ripple_pure_hvx_vshuff_i64(v32i64 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v64f32
ripple_pure_hvx_vshuff_f32(v64f32 src, size_t chunk_size);
extern RIPPLE_VECTOR_PAIR_CANDIDATE v32f64
ripple_pure_hvx_vshuff_f64(v32f64 src, size_t chunk_size);

#if __has_Float16__
extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v128f16 ripple_pure_hvx_vshuff_f16(v128f16 src, size_t chunk_sizze);
#endif

#if __has_bf16__
extern RIPPLE_VECTOR_PAIR_CANDIDATE
    v128bf16 ripple_pure_hvx_vshuff_bf16(v128bf16 src, size_t chunk_size);
#endif

#undef RIPPLE_VECTOR_PAIR_CANDIDATE

#define _decl_hvx_vdeals(ct, t)                                                \
  __attribute__((used, always_inline, weak)) extern ct hvx_vdeal_##t(          \
      ct src, size_t chunk_size);                                              \
                                                                               \
  __attribute__((used, always_inline, weak)) extern ct hvx_vshuff_##t(         \
      ct src, size_t chunk_size);

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

#define __hvx_vdeal_anyint(Type, T, chunk_size, val)                           \
  ((Type)(sizeof(Type) == 1                                                    \
              ? hvx_vdeal_##T##8((val), (chunk_size))                          \
              : (sizeof(Type) == 2                                             \
                     ? hvx_vdeal_##T##16((val), (chunk_size))                  \
                     : (sizeof(Type) == 4                                      \
                            ? hvx_vdeal_##T##32((val), (chunk_size))           \
                            : hvx_vdeal_##T##64((val), (chunk_size))))))

#define __hvx_vshuff_anyint(Type, T, chunk_size, val)                          \
  ((Type)(sizeof(Type) == 1                                                    \
              ? hvx_vshuff_##T##8((val), (chunk_size))                         \
              : (sizeof(Type) == 2                                             \
                     ? hvx_vshuff_##T##16((val), (chunk_size))                 \
                     : (sizeof(Type) == 4                                      \
                            ? hvx_vshuff_##T##32((val), (chunk_size))          \
                            : hvx_vshuff_##T##64((val), (chunk_size))))))

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
          ? __hvx_vdeal_anyint(char, i, chunk_size, src)                       \
          : __hvx_vdeal_anyint(char, u, chunk_size, src),                      \
      signed char: __hvx_vdeal_anyint(signed char, i, chunk_size, src),        \
      unsigned char: __hvx_vdeal_anyint(unsigned char, u, chunk_size, src),    \
      signed short: __hvx_vdeal_anyint(signed short, i, chunk_size, src),      \
      unsigned short: __hvx_vdeal_anyint(unsigned short, u, chunk_size, src),  \
      signed int: __hvx_vdeal_anyint(signed int, i, chunk_size, src),          \
      unsigned int: __hvx_vdeal_anyint(unsigned int, u, chunk_size, src),      \
      signed long: __hvx_vdeal_anyint(signed long, i, chunk_size, src),        \
      unsigned long: __hvx_vdeal_anyint(unsigned long, u, chunk_size, src),    \
      signed long long: __hvx_vdeal_anyint(signed long long, i, chunk_size,    \
                                           src),                               \
      unsigned long long: __hvx_vdeal_anyint(unsigned long long, u,            \
                                             chunk_size, src),                 \
      float: hvx_vdeal_f32((src), (chunk_size)),                               \
      double: hvx_vdeal_f64((src), (chunk_size))                               \
          hvx_vdeal_extra_f16(src, (chunk_size))                               \
              hvx_vdeal_extra_bf16(src, (chunk_size)))

#define __hvx_vshuff(src, chunk_size)                                          \
  _Generic((src),                                                              \
      char: __ripple_char_is_signed                                            \
          ? __hvx_vshuff_anyint(char, i, chunk_size, src)                      \
          : __hvx_vshuff_anyint(char, u, chunk_size, src),                     \
      signed char: __hvx_vshuff_anyint(signed char, i, chunk_size, src),       \
      unsigned char: __hvx_vshuff_anyint(unsigned char, u, chunk_size, src),   \
      signed short: __hvx_vshuff_anyint(signed short, i, chunk_size, src),     \
      unsigned short: __hvx_vshuff_anyint(unsigned short, u, chunk_size, src), \
      signed int: __hvx_vshuff_anyint(signed int, i, chunk_size, src),         \
      unsigned int: __hvx_vshuff_anyint(unsigned int, u, chunk_size, src),     \
      signed long: __hvx_vshuff_anyint(signed long, i, chunk_size, src),       \
      unsigned long: __hvx_vshuff_anyint(unsigned long, u, chunk_size, src),   \
      signed long long: __hvx_vshuff_anyint(signed long long, i, chunk_size,   \
                                            src),                              \
      unsigned long long: __hvx_vshuff_anyint(unsigned long long, u,           \
                                              chunk_size, src),                \
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
  [[gnu::always_inline]] static CT __hvx_vdeal(CT Val, size_t chunk_size) {    \
    switch (sizeof(Val)) {                                                     \
    case 1:                                                                    \
      return hvx_vdeal_##UI##8((Val), (chunk_size));                           \
    case 2:                                                                    \
      return hvx_vdeal_##UI##16((Val), (chunk_size));                          \
    case 4:                                                                    \
      return hvx_vdeal_##UI##32((Val), (chunk_size));                          \
    case 8:                                                                    \
      return hvx_vdeal_##UI##64((Val), (chunk_size));                          \
    default:                                                                   \
      __builtin_unreachable();                                                 \
    }                                                                          \
  }                                                                            \
                                                                               \
  [[gnu::always_inline]] static CT __hvx_vshuff(CT Val, size_t chunk_size) {   \
    switch (sizeof(Val)) {                                                     \
    case 1:                                                                    \
      return hvx_vshuff_##UI##8((Val), (chunk_size));                          \
    case 2:                                                                    \
      return hvx_vshuff_##UI##16((Val), (chunk_size));                         \
    case 4:                                                                    \
      return hvx_vshuff_##UI##32((Val), (chunk_size));                         \
    case 8:                                                                    \
      return hvx_vshuff_##UI##64((Val), (chunk_size));                         \
    default:                                                                   \
      __builtin_unreachable();                                                 \
    }                                                                          \
  }

_decl_hvx_vdeals_spec_int(signed char, i)
    _decl_hvx_vdeals_spec_int(unsigned char, u) _decl_hvx_vdeals_spec_int(short,
                                                                          i)
        _decl_hvx_vdeals_spec_int(unsigned short, u)
            _decl_hvx_vdeals_spec_int(int, i)
                _decl_hvx_vdeals_spec_int(unsigned int, u)
                    _decl_hvx_vdeals_spec_int(long, i)
                        _decl_hvx_vdeals_spec_int(unsigned long, u)
                            _decl_hvx_vdeals_spec_int(long long, i)
                                _decl_hvx_vdeals_spec_int(unsigned long long, u)

#undef _decl_hvx_vdeals_spec_int

    // plain char — signedness is implementation-defined
    [[gnu::always_inline]] static char __hvx_vdeal(const char Val,
                                                   size_t chunk_size) {
  return __ripple_char_is_signed ? hvx_vdeal_i8((Val), (chunk_size))
                                 : hvx_vdeal_u8((Val), (chunk_size));
}

[[gnu::always_inline]] static char __hvx_vshuff(const char Val,
                                                size_t chunk_size) {
  return __ripple_char_is_signed ? hvx_vshuff_i8((Val), (chunk_size))
                                 : hvx_vshuff_u8((Val), (chunk_size));
}

#if __has_Float16__
[[gnu::always_inline]] static _Float16 __hvx_vdeal(_Float16 Val,
                                                   size_t chunk_size) {
  return hvx_vdeal_f16((Val), (chunk_size));
}
[[gnu::always_inline]] static _Float16 __hvx_vshuff(_Float16 Val,
                                                    size_t chunk_size) {
  return hvx_vshuff_f16((Val), (chunk_size));
}
#endif

#if __has_bf16__
[[gnu::always_inline]] static __bf16 __hvx_vdeal(__bf16 Val,
                                                 size_t chunk_size) {
  return hvx_vdeal_bf16((Val), (chunk_size));
}
[[gnu::always_inline]] static __bf16 __hvx_vshuff(__bf16 Val,
                                                  size_t chunk_size) {
  return hvx_vshuff_bf16((Val), (chunk_size));
}
#endif

[[gnu::always_inline]] static float __hvx_vdeal(float Val, size_t chunk_size) {
  return hvx_vdeal_f32((Val), (chunk_size));
}
[[gnu::always_inline]] static float __hvx_vshuff(float Val, size_t chunk_size) {
  return hvx_vshuff_f32((Val), (chunk_size));
}

[[gnu::always_inline]] static double __hvx_vdeal(double Val,
                                                 size_t chunk_size) {
  return hvx_vdeal_f64((Val), (chunk_size));
}
[[gnu::always_inline]] static double __hvx_vshuff(double Val,
                                                  size_t chunk_size) {
  return hvx_vshuff_f64((Val), (chunk_size));
}

// ----------------------------------------------------------------------------
// Public C++ template definitions
// ----------------------------------------------------------------------------

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_chunked_unzip(T x, size_t chunk_size) {
  return __hvx_vdeal(x, -chunk_size);
}

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_chunked_zip(T x, size_t chunk_size) {
  return __hvx_vshuff(x, -chunk_size);
}

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_chunked_unzip_list(T x, size_t chunk_size, size_t list_size) {
  return __hvx_vdeal(x, list_size - chunk_size);
}

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_chunked_zip_list(T x, size_t chunk_size, size_t list_size) {
  return __hvx_vshuff(x, list_size - chunk_size);
}

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_2x2_transpose_dec(T x, size_t chunk_size) {
  return __hvx_vdeal(x, chunk_size);
}

template <typename T>
__attribute__((always_inline)) static T
hvx_pair_2x2_transpose_inc(T x, size_t chunk_size) {
  return __hvx_vshuff(x, chunk_size);
}

#endif // __cplusplus
