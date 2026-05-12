// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %clang++ -S -fenable-ripple -O2 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="warning:" --implicit-check-not="error:"s

#include <ripple.h>
#include <ripple_math.h>

#include "ripple_test.h"

void reinterp_i32_to_i8(const int32_t *input, int8_t *output) {
  auto BS_src = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS_src, 0);
  int32_t val = input[v0];

  // 32 x i32 = 128 bytes -> 128 x i8
  auto BS_dst = ripple_set_block_shape(0, 128);
  size_t v0_dst = ripple_id(BS_dst, 0);
  int8_t result = ripple_reinterp_i8(BS_dst, val);
  result = result * 2;
  output[v0_dst] = result;

  // CHECK: load <128 x i8>, ptr %input
}

void reinterp_i32_to_i16(const int32_t *input, int16_t *output) {
  auto BS_src = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS_src, 0);
  int32_t val = input[v0];

  // 32 x i32 = 128 bytes -> 64 x i16
  auto BS_dst = ripple_set_block_shape(0, 64);
  size_t v0_dst = ripple_id(BS_dst, 0);
  int16_t result = ripple_reinterp_i16(BS_dst, val);
  result = result * 2;
  output[v0_dst] = result;

  // CHECK: load <64 x i16>, ptr %input
}

void reinterp_i32_2d_to_i8(const int32_t *input, int8_t *output) {
  auto BS_src = ripple_set_block_shape(0, 32, 2);
  size_t v0 = ripple_id(BS_src, 0);
  size_t v1 = ripple_id(BS_src, 1);
  int32_t val = input[v1 * 32 + v0];

  auto BS_dst = ripple_set_block_shape(0, 256);
  size_t v0_dst = ripple_id(BS_dst, 0);
  int8_t result = ripple_reinterp_i8(BS_dst, val);
  result = result * 2;
  output[v0_dst] = result;

  // CHECK: load <256 x i8>, ptr %input
}

void reinterp_i32_to_i64(const int32_t *input, int64_t *output) {
  auto BS_src = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS_src, 0);
  int32_t val = input[v0];

  // 32 x i32 = 128 bytes -> 16 x i64
  auto BS_dst = ripple_set_block_shape(0, 16);
  size_t v0_dst = ripple_id(BS_dst, 0);
  int64_t result = ripple_reinterp_i64(BS_dst, val);
  result = result * 2;
  output[v0_dst] = result;

  // CHECK: load <16 x i64>, ptr %input
}

void reinterp_i16_to_u32(const int16_t *input, uint32_t *output) {
  auto BS_src = ripple_set_block_shape(0, 64);
  size_t v0 = ripple_id(BS_src, 0);
  int16_t val = input[v0];

  // 64 x i16 = 128 bytes -> 32 x u32
  auto BS_dst = ripple_set_block_shape(0, 32);
  size_t v0_dst = ripple_id(BS_dst, 0);
  uint32_t result = ripple_reinterp_u32(BS_dst, val);
  result = result * 2;
  output[v0_dst] = result;

  // CHECK: load <32 x i32>, ptr %input
}