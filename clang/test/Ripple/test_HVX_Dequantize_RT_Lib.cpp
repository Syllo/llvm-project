// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple -ffreestanding -nostdinc -isystem %resource_dir/ripple_include -isystem %resource_dir --target=hexagon -mhvx -mv81 -emit-llvm %s -o - 2>&1 | FileCheck %s

#include "ripple_test.h"
#include "ripple_hvx_test_types.h"
// HVX_Dequantize RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((always_inline)) extern uint32_t
hvx_dequantize_u8_to_f32(const uint8_t vin, const int16_t zero_offset,
                         const uint8_t from_qint8, const int32_t iscale,
                         const int16_t exp_base);
__attribute__((always_inline)) extern float
hvx_dequantize_u16_to_f32_flat(const uint16_t vin, uint32_t offset,
                               float scale);
__attribute__((always_inline)) extern uint32_t
hvx_dequantize_u16_to_f32(const uint16_t vin, uint16_t zero_offset,
                          uint16_t from_qint16, int32_t iscale,
                          int16_t exp_base);
extern RIPPLE_VECTOR_CANDIDATE v128u32 ripple_pure_ew_hvx_dequantize_u8_to_f32(
    v128u8 vin, const int16_t zero_offset, const uint8_t from_qint8,
    const int32_t iscale, const int16_t exp_base);
extern RIPPLE_VECTOR_CANDIDATE v64f32
ripple_pure_hvx_dequantize_u16_to_f32_flat(const v64u16 vin, uint32_t offset,
                                           float scale);
extern RIPPLE_VECTOR_CANDIDATE v64u32 ripple_pure_hvx_dequantize_u16_to_f32(
    const v64u16 vin, const uint16_t zero_offset, const uint16_t from_qint16,
    const int32_t iscale, const int16_t exp_base);
#ifdef __cplusplus
}
#endif


extern "C" {
// u8 to f32
void test_dequantize_u8_to_f32(size_t length, uint8_t *in, float *out, const int16_t zero_offset, const uint8_t from_qint8, const int32_t iscale, const int16_t exp_base) {
  // NOTE: ripple_parallel met error
  // ripple_block_t BS = ripple_set_block_shape(0, 128);
  // ripple_parallel(BS, 0);
  // for (size_t i = 0; i < length; i++) {
  //   uint32_t u = hvx_dequantize_u8_to_f32(in[i], zero_offset, from_qint8, iscale, exp_base);
  //   __builtin_memcpy(&out[i], &u, sizeof(float));
  // }
  // NOTE: Ripple Version
  int const nvecs = length >> 7;
  int const leftover = length & 127u;
  auto BS = ripple_set_block_shape(0, 128);

  size_t v = ripple_id(BS, 0);
  for (size_t n = 0; n < nvecs; n++) {
      out[v] = hvx_dequantize_u8_to_f32(in[v], zero_offset, from_qint8, iscale, exp_base);
      in += 128;
      out += 128;
  }
  if (leftover) {
      if (v < leftover) {
        out[v] = hvx_dequantize_u8_to_f32(in[v], zero_offset, from_qint8, iscale, exp_base);
      }
  }
}
// CHECK: @test_dequantize_u8_to_f32
// CHECK: call void @ripple_pure_ew_hvx_dequantize_u8_to_f32

// u16 to f32
void test_dequantize_u16_to_f32(size_t length, uint16_t *in, float *out, uint32_t offset, float scale) {
    int const nvecs = length >> 6;
    int const leftover = length & 63u;
    auto BS = ripple_set_block_shape(0, 64);

    size_t v = ripple_id(BS, 0);
    for (size_t n = 0; n < nvecs; n++) {
        out[v] = hvx_dequantize_u16_to_f32_flat(in[v], offset, scale);
        in += 64;
        out += 64;
    }
    if (leftover) {
        if (v < leftover) {
            out[v] = hvx_dequantize_u16_to_f32_flat(in[v], offset, scale);
        }
    }
}
// CHECK: @test_dequantize_u16_to_f32
// CHECK: call <64 x float> @ripple_pure_hvx_dequantize_u16_to_f32_flat

}
