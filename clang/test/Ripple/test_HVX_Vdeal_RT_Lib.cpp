// REQUIRES: target=hexagon-{{.*}}, has-ripple-hexagon-rtlib
// RUN: %clang++ -ffreestanding -g -S -fenable-ripple -mhvx -mv81 -emit-llvm %s -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s
// RUN: %clang++ -ffreestanding -g -S -x c -fenable-ripple -mhvx -mv81 -emit-llvm %s -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s

#include "ripple_test.h"

#include <ripple.h>
#include <ripple_math.h>
#include <ripple/HVX_VectorPair.h>

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
