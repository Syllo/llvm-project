// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clangxx --target=hexagon -mcpu=hexagonv81 -mhvx -fenable-ripple -mllvm -ripple-pad-to-target-simd -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}
// RUN: %if x86-registered-target %{ %clangxx --target=x86_64-unknown-linux-gnu -fenable-ripple -mllvm -ripple-pad-to-target-simd -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}
// RUN: %if aarch64-registered-target %{ %clangxx --target=aarch64-unknown-linux-gnu -fenable-ripple -mllvm -ripple-pad-to-target-simd -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}

#include "ripple_test.h"

static size_t shift2(size_t index, size_t size) {
  return (index + 2) % size;
}

extern "C" void check_shuffle_function_o0(int *in, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 4);
  int id = ripple_id(BS, 0);
  out[id] = ripple_shuffle(in[id], shift2);
}

// CHECK-LABEL: @check_shuffle_function_o0
// CHECK: shufflevector <{{(4|32)}} x i32> %{{.*}}, <{{(4|32)}} x i32> poison, <{{(4|32)}} x i32> <i32 2, i32 3, i32 0, i32 1
// CHECK: ret void
