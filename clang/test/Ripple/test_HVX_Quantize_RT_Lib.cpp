// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple -ffreestanding -nostdinc -isystem %resource_dir/ripple_include -isystem %resource_dir --target=hexagon -mhvx -mv81 -emit-llvm %s -o - 2>&1 | FileCheck %s


#include "ripple_test.h"
#include "ripple_hvx_test_types.h"
// HVX_Quantize RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

#ifdef __cplusplus
extern "C" {
#endif
__attribute__((always_inline)) extern uint8_t
hvx_quantize_f32_to_u8(float scale_f, int16_t out_offset, const float in);
__attribute__((always_inline)) extern uint16_t
hvx_quantize_f32_to_u16(float scale_f, int32_t out_offset, const float in);
extern RIPPLE_VECTOR_CANDIDATE v128u8 ripple_pure_ew_hvx_quantize_f32_to_u8(
    float scale_f, int16_t out_offset, const v128f32 in);
extern RIPPLE_VECTOR_CANDIDATE v64u16 ripple_pure_ew_hvx_quantize_f32_to_u16(
    float scale_f, int32_t out_offset, const v64f32 in);
#ifdef __cplusplus
}
#endif


extern "C" {

// f32 to u8
void test_quantize_f32_u8(size_t length, uint8_t *out, const float *in, float scale_f, int16_t out_offset) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  ripple_parallel(BS, 0);
  for (size_t i = 0; i < length; i++) {
    out[i] = hvx_quantize_f32_to_u8(scale_f, out_offset, in[i]);
  }
}
// CHECK: @test_quantize_f32_u8
// CHECK: call <128 x i8> @ripple_pure_ew_hvx_quantize_f32_to_u8

// f32 to u8
void test_quantize_f32_u16(size_t length, uint16_t *out, const float *in, float scale_f, int32_t out_offset) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  ripple_parallel(BS, 0);
  for (size_t i = 0; i < length; i++) {
    out[i] = hvx_quantize_f32_to_u16(scale_f, out_offset, in[i]);
  }
}
// CHECK: @test_quantize_f32_u16
// CHECK: call <64 x i16> @ripple_pure_ew_hvx_quantize_f32_to_u16


}
