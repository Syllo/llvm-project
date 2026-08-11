// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -mhvx-qfloat -emit-llvm %s -o - 2>&1 | FileCheck %s

#include "ripple_test.h"
// HVX_Scatter_Gather RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
#include "ripple_hvx_test_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _decl_hvx_gather(ct, t)                                                \
  __attribute__((always_inline)) extern void hvx_gather_##t(                   \
      ct *dst, const ct *src, int index, size_t region_size)

_decl_hvx_gather(int8_t, i8);
_decl_hvx_gather(uint8_t, u8);
_decl_hvx_gather(int16_t, i16);
_decl_hvx_gather(uint16_t, u16);
_decl_hvx_gather(int32_t, i32);
_decl_hvx_gather(uint32_t, u32);
_decl_hvx_gather(int64_t, i64);
_decl_hvx_gather(uint64_t, u64);

_decl_hvx_gather(float, f32);
_decl_hvx_gather(double, f64);
#if __has_bf16__
_decl_hvx_gather(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather(_Float16, f16);
#endif
#undef _decl_hvx_gather

#define _decl_hvx_gather_16(ct, t)                                             \
  __attribute__((always_inline)) extern void hvx_gather_##t##_16(              \
      ct *dst, const ct *src, int16_t index, size_t region_size)

_decl_hvx_gather_16(int8_t, i8);
_decl_hvx_gather_16(uint8_t, u8);

_decl_hvx_gather_16(int16_t, i16);
_decl_hvx_gather_16(uint16_t, u16);

#if __has_bf16__
_decl_hvx_gather_16(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather_16(_Float16, f16);
#endif

#undef _decl_hvx_gather_16

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
#define both_gathers(gather_fn, dst, src, index, region_size)                  \
  _Generic((index),                                                            \
      int16_t: gather_fn##_16((dst), (src), (index), (region_size)),           \
      int32_t: gather_fn((dst), (src), (index), (region_size)))

#if __has_bf16__
#define __extra_bf16_gather(dst, src, index, region_size)                      \
  , __bf16 : hvx_gather_bf16((dst), (src), (index), (region_size))
#else
#define __extra_bf16_gather(dst, src, index, region_size)
#endif

#if __has_Float16__
#define __extra_f16_gather(dst, src, index, region_size)                       \
  , _Float16 : hvx_gather_f16((dst), (src), (index), (region_size))
#else
#define __extra_f16_gather(dst, src, index, region_size)
#endif

#define hvx_gather(dst, src, index, region_size)                               \
  _Generic((src),                                                              \
      char *: hvx_gather_i8((dst), (src), (index), (region_size)),             \
      int16_t *: both_gathers(hvx_gather_i16, (dst), (src), (index),           \
                              (region_size)),                                  \
      int32_t *: hvx_gather_i32((dst), (src), (index), (region_size)),         \
      int64_t *: hvx_gather_i64((dst), (src), (index), (region_size)),         \
      uint8_t *: hvx_gather_u8((dst), (src), (index), (region_size)),          \
      uint16_t *: both_gathers(hvx_gather_u16, (dst), (src), (index),          \
                               (region_size)),                                 \
      uint32_t *: hvx_gather_u32((dst), (src), (index), (region_size)),        \
      uint64_t *: hvx_gather_u64((dst), (src), (index), (region_size)),        \
      float *: hvx_gather_f32((dst), (src), (index), (region_size)),           \
      double *: hvx_gather_f64((dst), (src), (index), (region_size))           \
          __extra_bf16_gather(dst, src, index, region_size)                    \
              __extra_f16_gather(dst, src, index, region_size))

#else

#define _decl_hvx_gather_spec(ct, t)                                           \
  [[gnu::always_inline, gnu::unused]] static void hvx_gather(                  \
      ct *dst, const ct *src, int index, size_t region_size) {                 \
    hvx_gather_##t(dst, src, index, region_size);                              \
  }

_decl_hvx_gather_spec(int8_t, i8);
_decl_hvx_gather_spec(uint8_t, u8);
_decl_hvx_gather_spec(int16_t, i16);
_decl_hvx_gather_spec(uint16_t, u16);
_decl_hvx_gather_spec(int32_t, i32);
_decl_hvx_gather_spec(uint32_t, u32);
_decl_hvx_gather_spec(int64_t, i64);
_decl_hvx_gather_spec(uint64_t, u64);

_decl_hvx_gather_spec(float, f32);
_decl_hvx_gather_spec(double, f64);
#if __has_bf16__
_decl_hvx_gather_spec(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather_spec(_Float16, f16);
#endif

#undef _decl_hvx_gather_spec

#define _decl_hvx_gather_16_spec(ct, t)                                        \
  [[gnu::always_inline, gnu::unused]] static void hvx_gather(                  \
      ct *dst, const ct *src, int16_t index, size_t region_size) {             \
    hvx_gather_##t##_16(dst, src, index, region_size);                         \
  }

_decl_hvx_gather_16_spec(int16_t, i16);
_decl_hvx_gather_16_spec(uint16_t, u16);

_decl_hvx_gather_16_spec(int8_t, i8);
_decl_hvx_gather_16_spec(uint8_t, u8);

#if __has_bf16__
_decl_hvx_gather_16_spec(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather_16_spec(_Float16, f16);
#endif

#undef _decl_hvx_gather_16_spec

#endif

#ifdef __cplusplus
extern "C" {
#endif
#define _decl_hvx_scatter(w)                                                   \
  __attribute__((always_inline)) extern void hvx_scatter_i##w(                 \
      int##w##_t *dst, int index, int##w##_t src, size_t region_size);         \
  __attribute__((always_inline)) extern void hvx_scatter_u##w(                 \
      uint##w##_t *dst, int index, uint##w##_t src, size_t region_size)

#define _decl_hvx_scatter_16(w)                                                \
  __attribute__((always_inline)) extern void hvx_scatter_i##w##_16(            \
      int##w##_t *dst, int16_t index, int##w##_t src, size_t region_size);     \
  __attribute__((always_inline)) extern void hvx_scatter_u##w##_16(            \
      uint##w##_t *dst, int16_t index, uint##w##_t src, size_t region_size)

#define _decl_f_hvx_scatter(w, ct)                                             \
  __attribute__((always_inline)) extern void hvx_scatter_f##w(                 \
      ct *dst, int index, ct src, size_t region_size)

#define _decl_f_hvx_scatter_16(w, ct)                                          \
  __attribute__((always_inline)) extern void hvx_scatter_f##w##_16(            \
      ct *dst, int16_t index, ct src, size_t region_size);

#define _decl_f_hvx_scatter_bf16                                               \
  __attribute__((always_inline)) extern void hvx_scatter_bf16(                 \
      __bf16 *dst, int index, __bf16 src, size_t region_size)

#define _decl_f_hvx_scatter_bf16_16                                            \
  __attribute__((always_inline)) extern void hvx_scatter_bf16_16(              \
      __bf16 *dst, int16_t index, __bf16 src, size_t region_size)

_decl_hvx_scatter(8);
_decl_hvx_scatter(16);
_decl_hvx_scatter(32);
_decl_hvx_scatter(64);
_decl_hvx_scatter_16(8);
_decl_hvx_scatter_16(16);
#if __has_bf16__
_decl_f_hvx_scatter_bf16;
_decl_f_hvx_scatter_bf16_16;
#endif
#if __has_Float16__
_decl_f_hvx_scatter(16, _Float16);
_decl_f_hvx_scatter_16(16, _Float16);
#endif
_decl_f_hvx_scatter(32, float);
_decl_f_hvx_scatter(64, double);

#undef _decl_hvx_scatter
#undef _decl_hvx_scatter_16
#undef _decl_f_hvx_scatter

extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_i64(int64_t *dst,
                                                          const int64_t *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i64(int64_t *dst, const int64_t *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_i32(int32_t *dst,
                                                          const int32_t *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i32(int32_t *dst, const int32_t *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_i16(int16_t *dst,
                                                          const int16_t *src,
                                                          v64i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i16(int16_t *dst, const int16_t *src, v64i32 index,
                           size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_i8(int8_t *dst,
                                                         const int8_t *src,
                                                         v128i32 index,
                                                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i8(int8_t *dst, const int8_t *src, v128i32 index,
                          size_t region_size, v128i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_gather_i16_16(int16_t *dst, const int16_t *src, v64i16 index,
                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i16_16(int16_t *dst, const int16_t *src, v64i16 index,
                              size_t region_size, v64i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_i8_16(int8_t *dst,
                                                            const int8_t *src,
                                                            v128i16 index,
                                                            size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_i8_16(int8_t *dst, const int8_t *src, v128i16 index,
                             size_t region_size, v128i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_f64(double *dst,
                                                          const double *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_f64(double *dst, const double *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_f32(float *dst,
                                                          const float *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_f32(float *dst, const float *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_f16(_Float16 *dst,
                                                          const _Float16 *src,
                                                          v64i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_f16(_Float16 *dst, const _Float16 *src, v64i32 index,
                           size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_gather_f16_16(_Float16 *dst, const _Float16 *src, v64i16 index,
                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_f16_16(_Float16 *dst, const _Float16 *src, v64i16 index,
                              size_t region_size, v64i16 mask);
#if __has_bf16__
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_bf16(__bf16 *dst,
                                                           const __bf16 *src,
                                                           v64i32 index,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_bf16(__bf16 *dst, const __bf16 *src, v64i32 index,
                            size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_gather_bf16_16(__bf16 *dst, const __bf16 *src, v64i16 index,
                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_bf16_16(__bf16 *dst, const __bf16 *src, v64i16 index,
                               size_t region_size, v64i16 mask);
#endif
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_u64(int64_t *dst,
                                                          const int64_t *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u64(int64_t *dst, const int64_t *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_u32(int32_t *dst,
                                                          const int32_t *src,
                                                          v32i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u32(int32_t *dst, const int32_t *src, v32i32 index,
                           size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_u16(int16_t *dst,
                                                          const int16_t *src,
                                                          v64i32 index,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u16(int16_t *dst, const int16_t *src, v64i32 index,
                           size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_u8(int8_t *dst,
                                                         const int8_t *src,
                                                         v128i32 index,
                                                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u8(int8_t *dst, const int8_t *src, v128i32 index,
                          size_t region_size, v128i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_gather_u16_16(int16_t *dst, const int16_t *src, v64i16 index,
                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u16_16(int16_t *dst, const int16_t *src, v64i16 index,
                              size_t region_size, v64i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_gather_u8_16(int8_t *dst,
                                                            const int8_t *src,
                                                            v128i16 index,
                                                            size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_gather_u8_16(int8_t *dst, const int8_t *src, v128i16 index,
                             size_t region_size, v128i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_i64(int64_t *dst,
                                                           v32i32 index,
                                                           v32i64 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i64(int64_t *dst, v32i32 index, v32i64 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_i32(int32_t *dst,
                                                           v32i32 index,
                                                           v32i32 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i32(int32_t *dst, v32i32 index, v32i32 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_i16(int16_t *dst,
                                                           v64i32 index,
                                                           v64i16 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i16(int16_t *dst, v64i32 index, v64i16 src,
                            size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_i8(int8_t *dst,
                                                          v128i32 index,
                                                          v128i8 src,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i8(int8_t *dst, v128i32 index, v128i8 src,
                           size_t region_size, v128i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_i16_16(int16_t *dst, v64i16 index, v64i16 src,
                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i16_16(int16_t *dst, v64i16 index, v64i16 src,
                               size_t region_size, v64i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_i8_16(int8_t *dst, v128i16 index, v128i8 src,
                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_i8_16(int8_t *dst, v128i16 index, v128i8 src,
                              size_t region_size, v128i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_f64(double *dst,
                                                           v32i32 index,
                                                           v32f64 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_f64(double *dst, v32i32 index, v32f64 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_f32(float *dst,
                                                           v32i32 index,
                                                           v32f32 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_f32(float *dst, v32i32 index, v32f32 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_f16(_Float16 *dst,
                                                           v64i32 index,
                                                           v64f16 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_f16(_Float16 *dst, v64i32 index, v64f16 src,
                            size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_f16_16(_Float16 *dst, v64i16 index, v64f16 src,
                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_f16_16(_Float16 *dst, v64i16 index, v64f16 src,
                               size_t region_size, v64i16 mask);
#if __has_bf16__
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_bf16(__bf16 *dst,
                                                            v64i32 index,
                                                            v64bf16 src,
                                                            size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_bf16(__bf16 *dst, v64i32 index, v64bf16 src,
                             size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_bf16_16(__bf16 *dst, v64i16 index, v64bf16 src,
                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_bf16_16(__bf16 *dst, v64i16 index, v64bf16 src,
                                size_t region_size, v64i16 mask);
#endif
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_u64(int64_t *dst,
                                                           v32i32 index,
                                                           v32i64 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u64(int64_t *dst, v32i32 index, v32i64 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_u32(int32_t *dst,
                                                           v32i32 index,
                                                           v32i32 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u32(int32_t *dst, v32i32 index, v32i32 src,
                            size_t region_size, v32i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_u16(int16_t *dst,
                                                           v64i32 index,
                                                           v64i16 src,
                                                           size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u16(int16_t *dst, v64i32 index, v64i16 src,
                            size_t region_size, v64i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void ripple_hvx_scatter_u8(int8_t *dst,
                                                          v128i32 index,
                                                          v128i8 src,
                                                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u8(int8_t *dst, v128i32 index, v128i8 src,
                           size_t region_size, v128i32 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_u16_16(int16_t *dst, v64i16 index, v64i16 src,
                          size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u16_16(int16_t *dst, v64i16 index, v64i16 src,
                               size_t region_size, v64i16 mask);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_hvx_scatter_u8_16(int8_t *dst, v128i16 index, v128i8 src,
                         size_t region_size);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_mask_hvx_scatter_u8_16(int8_t *dst, v128i16 index, v128i8 src,
                              size_t region_size, v128i16 mask);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
#define both_scatters(scatter_fn, dst, index, src, region_size)                \
  _Generic((index),                                                            \
      int16_t: scatter_fn##_16((dst), (index), (src), (region_size)),          \
      int32_t: scatter_fn((dst), (index), (src), (region_size)))

#if __has_bf16__
#define __extra_bf16_scatter(dst, index, src, region_size)                     \
  , __bf16 : hvx_scatter_bf16((dst), (index), (src), (region_size))
#else
#define __extra_bf16_scatter(dst, index, src, region_size)
#endif

#if __has_Float16__
#define __extra_f16_scatter(dst, index, src, region_size)                      \
  , _Float16 : hvx_scatter_f16((dst), (index), (src), (region_size))
#else
#define __extra_f16_scatter(dst, index, src, region_size)
#endif

#define hvx_scatter(dst, index, src, region_size)                              \
  _Generic((src),                                                              \
      int8_t: hvx_scatter_i8((dst), (index), (src), (region_size)),            \
      int16_t: both_scatters(hvx_scatter_i16, dst, index, src, region_size),   \
      int32_t: hvx_scatter_i32((dst), (index), (src), (region_size)),          \
      int64_t: hvx_scatter_i64((dst), (index), (src), (region_size)),          \
      uint8_t: hvx_scatter_u8((dst), (index), (src), (region_size)),           \
      uint16_t: both_scatters(hvx_scatter_u16, dst, index, src, region_size),  \
      uint32_t: hvx_scatter_u32((dst), (index), (src), (region_size)),         \
      uint64_t: hvx_scatter_u64((dst), (index), (src), (region_size)),         \
      float: hvx_scatter_f32((dst), (index), (src), (region_size)),            \
      double: hvx_scatter_f64((dst), (index), (src), (region_size))            \
          __extra_bf16_scatter(dst, index, src, region_size)                   \
              __extra_f16_scatter(dst, index, src, region_size))

#else
#define _decl_hvx_scatter_spec(ct, t)                                          \
  [[gnu::always_inline, gnu::unused]] static void hvx_scatter(                 \
      ct *dst, int index, ct src, size_t region_size) {                        \
    hvx_scatter_##t(dst, index, src, region_size);                             \
  }

_decl_hvx_scatter_spec(int8_t, i8);
_decl_hvx_scatter_spec(uint8_t, u8);
_decl_hvx_scatter_spec(int16_t, i16);
_decl_hvx_scatter_spec(uint16_t, u16);
_decl_hvx_scatter_spec(int32_t, i32);
_decl_hvx_scatter_spec(uint32_t, u32);
_decl_hvx_scatter_spec(int64_t, i64);
_decl_hvx_scatter_spec(uint64_t, u64);

_decl_hvx_scatter_spec(float, f32);
_decl_hvx_scatter_spec(double, f64);
#if __has_bf16__
_decl_hvx_scatter_spec(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_scatter_spec(_Float16, f16);
#endif

#undef _decl_hvx_scatter_spec

#define _decl_hvx_scatter_16_spec(ct, t)                                       \
  [[gnu::always_inline, gnu::unused]] static void hvx_scatter(                 \
      ct *dst, int16_t index, ct src, size_t region_size) {                    \
    hvx_scatter_##t##_16(dst, index, src, region_size);                        \
  }

_decl_hvx_scatter_16_spec(int8_t, i8);
_decl_hvx_scatter_16_spec(uint8_t, u8);

_decl_hvx_scatter_16_spec(int16_t, i16);
_decl_hvx_scatter_16_spec(uint16_t, u16);

#if __has_bf16__
_decl_hvx_scatter_16_spec(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_scatter_16_spec(_Float16, f16);
#endif

#undef _decl_hvx_scatter_16_spec

#endif

#define HVX_128_i8 128
#define HVX_64_i16 64
#define HVX_32_i32 32

extern "C" {
void Ripple_gather_u32(size_t length, uint32_t *destination, uint32_t *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_u32
// CHECK: call void @ripple_hvx_gather_u32
// CHECK: call void @ripple_mask_hvx_gather_u32

void Ripple_gather_i32(size_t length, int32_t *destination, int32_t *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i32
// CHECK: call void @ripple_hvx_gather_i32
// CHECK: call void @ripple_mask_hvx_gather_i32

void Ripple_gather_i64(size_t length, int64_t *destination, int64_t *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i64
// CHECK: call void @ripple_hvx_gather_i64
// CHECK: call void @ripple_mask_hvx_gather_i64

void Ripple_gather_i16(size_t length, int16_t *destination, int16_t *source,
                       int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i16
// CHECK: call void @ripple_hvx_gather_i16
// CHECK: call void @ripple_mask_hvx_gather_i16

void Ripple_gather_i8(size_t length, int8_t *destination, int8_t *source,
                      int8_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i8
// CHECK: call void @ripple_hvx_gather_i8
// CHECK: call void @ripple_mask_hvx_gather_i8

void Ripple_gather_i16_16(size_t length, int16_t *destination, int16_t *source,
                          int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i16_16
// CHECK: call void @ripple_hvx_gather_i16
// CHECK: call void @ripple_mask_hvx_gather_i16

void Ripple_gather_i8_16(size_t length, int8_t *destination, int8_t *source,
                         int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_i8_16
// CHECK: call void @ripple_hvx_gather_i8
// CHECK: call void @ripple_mask_hvx_gather_i8

void Ripple_gather_f32(size_t length, float *destination, float *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_f32
// CHECK: call void @ripple_hvx_gather_f32
// CHECK: call void @ripple_mask_hvx_gather_f32

void Ripple_gather_f16(size_t length, _Float16 *destination, _Float16 *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_f16
// CHECK: call void @ripple_hvx_gather_f16
// CHECK: call void @ripple_mask_hvx_gather_f16

void Ripple_gather_f64(size_t length, double *destination, double *source,
                       int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
  if (i + v < length) {
    hvx_gather(destination + i, source, indexes[i + v], length);
  }
}
// CHECK: @Ripple_gather_f64
// CHECK: call void @ripple_hvx_gather_f64
// CHECK: call void @ripple_mask_hvx_gather_f64

void Ripple_scatter_i64(size_t length, int64_t *destination, int64_t *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i64
// CHECK: call void @ripple_hvx_scatter_i64
// CHECK: call void @ripple_mask_hvx_scatter_i64

void Ripple_scatter_i32(size_t length, int32_t *destination, int32_t *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i32
// CHECK: call void @ripple_hvx_scatter_i32
// CHECK: call void @ripple_mask_hvx_scatter_i32

void Ripple_scatter_i16(size_t length, int16_t *destination, int16_t *source,
                        int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i16
// CHECK: call void @ripple_hvx_scatter_i16
// CHECK: call void @ripple_mask_hvx_scatter_i16

void Ripple_scatter_i8(size_t length, int8_t *destination, int8_t *source,
                       int8_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i8
// CHECK: call void @ripple_hvx_scatter_i8
// CHECK: call void @ripple_mask_hvx_scatter_i8

void Ripple_scatter_i16_16(size_t length, int16_t *destination, int16_t *source,
                           int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i16_16
// CHECK: call void @ripple_hvx_scatter_i16
// CHECK: call void @ripple_mask_hvx_scatter_i16

void Ripple_scatter_i8_16(size_t length, int8_t *destination, int8_t *source,
                          int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_i8_16
// CHECK: call void @ripple_hvx_scatter_i8
// CHECK: call void @ripple_mask_hvx_scatter_i8

void Ripple_scatter_u64(size_t length, uint64_t *destination, uint64_t *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u64
// CHECK: call void @ripple_hvx_scatter_u64
// CHECK: call void @ripple_mask_hvx_scatter_u64

void Ripple_scatter_u32(size_t length, uint32_t *destination, uint32_t *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u32
// CHECK: call void @ripple_hvx_scatter_u32
// CHECK: call void @ripple_mask_hvx_scatter_u32

void Ripple_scatter_u16(size_t length, uint16_t *destination, uint16_t *source,
                        int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u16
// CHECK: call void @ripple_hvx_scatter_u16
// CHECK: call void @ripple_mask_hvx_scatter_u16

void Ripple_scatter_u8(size_t length, uint8_t *destination, uint8_t *source,
                       uint8_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u8
// CHECK: call void @ripple_hvx_scatter_u8
// CHECK: call void @ripple_mask_hvx_scatter_u8

void Ripple_scatter_u16_16(size_t length, uint16_t *destination,
                           uint16_t *source, int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u16
// CHECK: call void @ripple_hvx_scatter_u16
// CHECK: call void @ripple_mask_hvx_scatter_u16

void Ripple_scatter_u8_16(size_t length, uint8_t *destination, uint8_t *source,
                          int16_t *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_128_i8);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_u8_16
// CHECK: call void @ripple_hvx_scatter_u8
// CHECK: call void @ripple_mask_hvx_scatter_u8

void Ripple_scatter_f32(size_t length, float *destination, float *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_f32
// CHECK: call void @ripple_hvx_scatter_f32
// CHECK: call void @ripple_mask_hvx_scatter_f32

void Ripple_scatter_f16(size_t length, _Float16 *destination, _Float16 *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_64_i16);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_f16
// CHECK: call void @ripple_hvx_scatter_f16
// CHECK: call void @ripple_mask_hvx_scatter_f16

void Ripple_scatter_f64(size_t length, double *destination, double *source,
                        int *indexes) {
  ripple_block_t BS = ripple_set_block_shape(0, HVX_32_i32);
  int nv = ripple_get_block_size(BS, 0);
  int v = ripple_id(BS, 0);
  int i;

  for (i = 0; i + nv <= length; i += nv) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
  if (i + v < length) {
    hvx_scatter(destination, indexes[i + v], source[i + v], length);
  }
}
// CHECK: @Ripple_scatter_f64
// CHECK: call void @ripple_hvx_scatter_f64
// CHECK: call void @ripple_mask_hvx_scatter_f64
}
