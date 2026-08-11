// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -mhvx-qfloat -emit-llvm %s -mllvm -print-after=ripple -o - 2>&1 | FileCheck %s
// RUN: %clang++ -g -S -x c -fenable-ripple --target=hexagon -mhvx -mv81 -mhvx-qfloat -emit-llvm %s -mllvm -print-after=ripple -o - 2>&1 | FileCheck %s

#include "ripple_test.h"

#include <ripple.h>
// HVX_VectorPair RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

#include "ripple_hvx_test_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern RIPPLE_VECTOR_CANDIDATE
    v256u8 ripple_pure_hvx_vdeal_u8(v256u8 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_pure_hvx_vdeal_u16(v128u16 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_pure_hvx_vdeal_u32(v64u32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32u64
ripple_pure_hvx_vdeal_u64(v32u64 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v256i8
ripple_pure_hvx_vdeal_i8(v256i8 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_pure_hvx_vdeal_i16(v128i16 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_pure_hvx_vdeal_i32(v64i32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32i64
ripple_pure_hvx_vdeal_i64(v32i64 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64f32
ripple_pure_hvx_vdeal_f32(v64f32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32f64
ripple_pure_hvx_vdeal_f64(v32f64 src, size_t chunk_size);

#if __has_Float16__
extern RIPPLE_VECTOR_CANDIDATE
    v128f16 ripple_pure_hvx_vdeal_f16(v128f16 src, size_t chunk_sizze);
#endif

#if __has_bf16__
extern RIPPLE_VECTOR_CANDIDATE
    v128bf16 ripple_pure_hvx_vdeal_bf16(v128bf16 src, size_t chunk_size);
#endif

extern RIPPLE_VECTOR_CANDIDATE
    v256u8 ripple_pure_hvx_vshuff_u8(v256u8 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_pure_hvx_vshuff_u16(v128u16 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_pure_hvx_vshuff_u32(v64u32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32u64
ripple_pure_hvx_vshuff_u64(v32u64 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v256i8
ripple_pure_hvx_vshuff_i8(v256i8 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_pure_hvx_vshuff_i16(v128i16 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_pure_hvx_vshuff_i32(v64i32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32i64
ripple_pure_hvx_vshuff_i64(v32i64 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v64f32
ripple_pure_hvx_vshuff_f32(v64f32 src, size_t chunk_size);
extern RIPPLE_VECTOR_CANDIDATE v32f64
ripple_pure_hvx_vshuff_f64(v32f64 src, size_t chunk_size);

#if __has_Float16__
extern RIPPLE_VECTOR_CANDIDATE
    v128f16 ripple_pure_hvx_vshuff_f16(v128f16 src, size_t chunk_sizze);
#endif

#if __has_bf16__
extern RIPPLE_VECTOR_CANDIDATE
    v128bf16 ripple_pure_hvx_vshuff_bf16(v128bf16 src, size_t chunk_size);
#endif

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
}
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

#endif

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

#endif

#define HVX_128_i8 256
#define HVX_64_i16 128
#define HVX_32_i32 64
#define HVX_16_i64 32

#define HVX_128_u8 256
#define HVX_64_u16 128
#define HVX_32_u32 64
#define HVX_16_u64 32

#define HVX_16_f64 32
#define HVX_32_f32 64
#define HVX_64_bf16 128
#define HVX_64_f16 128

#define __gen_vdeal_tests(CT, T, N)                                            \
    void Ripple_chunked_zip_##T(size_t length, CT *dest,                       \
        CT *src, size_t chunk_size) {                                          \
      ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);            \
      int v = ripple_id(BS, 0);                                                \
      src[v] = src[v] + src[v];                                                \
      dest[v] = hvx_pair_chunked_zip(src[v], chunk_size);                      \
    }                                                                          \
  \
    void Ripple_chunked_unzip_##T(size_t length, CT *dest,                     \
        CT *src, size_t chunk_size) {                                          \
      ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);            \
      int v = ripple_id(BS, 0);                                                \
      src[v] = src[v] + src[v];                                                \
      dest[v] = hvx_pair_chunked_unzip(src[v], chunk_size);                    \
    }                                                                          \
  \
    void Ripple_chunked_zip_list_##T(size_t length, CT *dest,                  \
      CT *src, size_t chunk_size, size_t list_size) {                          \
        ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);          \
        int v = ripple_id(BS, 0);                                              \
        src[v] = src[v] + src[v];                                              \
        dest[v] = hvx_pair_chunked_zip_list(src[v], chunk_size, list_size);    \
      }                                                                        \
  \
    void Ripple_chunked_unzip_list_##T(size_t length, CT *dest,                \
      CT *src, size_t chunk_size, size_t list_size) {                          \
        ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);          \
        int v = ripple_id(BS, 0);                                              \
        src[v] = src[v] + src[v];                                              \
        dest[v] = hvx_pair_chunked_unzip_list(src[v], chunk_size, list_size);  \
      }                                                                        \
  \
    void Ripple_transpose_dec_##T(size_t length, CT *dest,                     \
      CT *src, size_t chunk_size) {                                            \
        ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);          \
        int v = ripple_id(BS, 0);                                              \
        src[v] = src[v] + src[v];                                              \
        dest[v] = hvx_pair_2x2_transpose_dec(src[v], chunk_size);              \
      }                                                                        \
  \
    void Ripple_transpose_inc_##T(size_t length, CT *dest,                     \
      CT *src, size_t chunk_size) {                                            \
        ripple_block_t BS = ripple_set_block_shape(0, HVX_##N##_##T);          \
        int v = ripple_id(BS, 0);                                              \
        src[v] = src[v] + src[v];                                              \
        dest[v] = hvx_pair_2x2_transpose_inc(src[v], chunk_size);              \
      }

#ifdef __cplusplus
extern "C" {
#endif

__gen_vdeal_tests(int8_t, i8, 128);
// CHECK: @Ripple_chunked_zip_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_i8
// CHECK: @Ripple_chunked_unzip_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_i8
// CHECK: @Ripple_chunked_zip_list_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_i8
// CHECK: @Ripple_chunked_unzip_list_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_i8
// CHECK: @Ripple_transpose_dec_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_i8
// CHECK: @Ripple_transpose_inc_i8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_i8

__gen_vdeal_tests(uint8_t, u8, 128);
// CHECK: @Ripple_chunked_zip_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_u8
// CHECK: @Ripple_chunked_unzip_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_u8
// CHECK: @Ripple_chunked_zip_list_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_u8
// CHECK: @Ripple_chunked_unzip_list_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_u8
// CHECK: @Ripple_transpose_dec_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vdeal_u8
// CHECK: @Ripple_transpose_inc_u8
// CHECK: call <256 x i8> @ripple_pure_hvx_vshuff_u8

__gen_vdeal_tests(int16_t, i16, 64);
// CHECK: @Ripple_chunked_zip_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_i16
// CHECK: @Ripple_chunked_unzip_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_i16
// CHECK: @Ripple_chunked_zip_list_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_i16
// CHECK: @Ripple_chunked_unzip_list_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_i16
// CHECK: @Ripple_transpose_dec_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_i16
// CHECK: @Ripple_transpose_inc_i16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_i16

__gen_vdeal_tests(uint16_t, u16, 64);
// CHECK: @Ripple_chunked_zip_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_u16
// CHECK: @Ripple_chunked_unzip_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_u16
// CHECK: @Ripple_chunked_zip_list_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_u16
// CHECK: @Ripple_chunked_unzip_list_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_u16
// CHECK: @Ripple_transpose_dec_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vdeal_u16
// CHECK: @Ripple_transpose_inc_u16
// CHECK: call <128 x i16> @ripple_pure_hvx_vshuff_u16

__gen_vdeal_tests(int32_t, i32, 32);
// CHECK: @Ripple_chunked_zip_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_i32
// CHECK: @Ripple_chunked_unzip_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_i32
// CHECK: @Ripple_chunked_zip_list_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_i32
// CHECK: @Ripple_chunked_unzip_list_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_i32
// CHECK: @Ripple_transpose_dec_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_i32
// CHECK: @Ripple_transpose_inc_i32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_i32

__gen_vdeal_tests(uint32_t, u32, 32);
// CHECK: @Ripple_chunked_zip_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_u32
// CHECK: @Ripple_chunked_unzip_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_u32
// CHECK: @Ripple_chunked_zip_list_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_u32
// CHECK: @Ripple_chunked_unzip_list_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_u32
// CHECK: @Ripple_transpose_dec_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vdeal_u32
// CHECK: @Ripple_transpose_inc_u32
// CHECK: call <64 x i32> @ripple_pure_hvx_vshuff_u32

__gen_vdeal_tests(int64_t, i64, 16);
// CHECK: @Ripple_chunked_zip_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_i64
// CHECK: @Ripple_chunked_unzip_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_i64
// CHECK: @Ripple_chunked_zip_list_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_i64
// CHECK: @Ripple_chunked_unzip_list_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_i64
// CHECK: @Ripple_transpose_dec_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_i64
// CHECK: @Ripple_transpose_inc_i64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_i64

__gen_vdeal_tests(uint64_t, u64, 16);
// CHECK: @Ripple_chunked_zip_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_u64
// CHECK: @Ripple_chunked_unzip_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_u64
// CHECK: @Ripple_chunked_zip_list_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_u64
// CHECK: @Ripple_chunked_unzip_list_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_u64
// CHECK: @Ripple_transpose_dec_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vdeal_u64
// CHECK: @Ripple_transpose_inc_u64
// CHECK: call <32 x i64> @ripple_pure_hvx_vshuff_u64

__gen_vdeal_tests(float, f32, 32);
// CHECK: @Ripple_chunked_zip_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vshuff_f32
// CHECK: @Ripple_chunked_unzip_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vdeal_f32
// CHECK: @Ripple_chunked_zip_list_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vshuff_f32
// CHECK: @Ripple_chunked_unzip_list_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vdeal_f32
// CHECK: @Ripple_transpose_dec_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vdeal_f32
// CHECK: @Ripple_transpose_inc_f32
// CHECK: call <64 x float> @ripple_pure_hvx_vshuff_f32

__gen_vdeal_tests(double, f64, 16);
// CHECK: @Ripple_chunked_zip_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vshuff_f64
// CHECK: @Ripple_chunked_unzip_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vdeal_f64
// CHECK: @Ripple_chunked_zip_list_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vshuff_f64
// CHECK: @Ripple_chunked_unzip_list_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vdeal_f64
// CHECK: @Ripple_transpose_dec_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vdeal_f64
// CHECK: @Ripple_transpose_inc_f64
// CHECK: call <32 x double> @ripple_pure_hvx_vshuff_f64

#if __has_Float16__
__gen_vdeal_tests(_Float16, f16, 64);
// CHECK: @Ripple_chunked_zip_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vshuff_f16
// CHECK: @Ripple_chunked_unzip_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vdeal_f16
// CHECK: @Ripple_chunked_zip_list_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vshuff_f16
// CHECK: @Ripple_chunked_unzip_list_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vdeal_f16
// CHECK: @Ripple_transpose_dec_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vdeal_f16
// CHECK: @Ripple_transpose_inc_f16
// CHECK: call <128 x half> @ripple_pure_hvx_vshuff_f16
#endif

#if __has_bf16__
__gen_vdeal_tests(__bf16, bf16, 64);
// CHECK: @Ripple_chunked_zip_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vshuff_bf16
// CHECK: @Ripple_chunked_unzip_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vdeal_bf16
// CHECK: @Ripple_chunked_zip_list_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vshuff_bf16
// CHECK: @Ripple_chunked_unzip_list_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vdeal_bf16
// CHECK: @Ripple_transpose_dec_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vdeal_bf16
// CHECK: @Ripple_transpose_inc_bf16
// CHECK: call <128 x bfloat> @ripple_pure_hvx_vshuff_bf16
#endif

#ifdef __cplusplus
}
#endif
