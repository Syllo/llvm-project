// REQUIRES: hexagon-registered-target, has-ripple-hexagon-rtlib
// RUN: %clang++ -g -S -fenable-ripple -ffreestanding -nostdinc -isystem %resource_dir/ripple_include -isystem %resource_dir --target=hexagon -mhvx -mv79 -emit-llvm %s -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s

#include "ripple_test.h"
#include <ripple_hvx.h>
#include <ripple/HVX_Dequantize.h>


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
