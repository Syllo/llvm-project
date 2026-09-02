// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clangxx --target=hexagon -fenable-ripple -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}
// RUN: %if x86-registered-target %{ %clangxx --target=x86_64-unknown-linux-gnu -fenable-ripple -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}
// RUN: %if aarch64-registered-target %{ %clangxx --target=aarch64-unknown-linux-gnu -fenable-ripple -S -O0 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="error:" %}

#include "ripple_test.h"

extern "C" void check_shuffle_lambda_o0(int *in, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 4);
  int id = ripple_id(BS, 0);
  out[id] = ripple_shuffle(in[id], [](size_t index, size_t size) {
    return (index + 1) % size;
  });
}

// CHECK-LABEL: @check_shuffle_lambda_o0
// CHECK: shufflevector <4 x i32> %{{.*}}, <4 x i32> poison, <4 x i32> <i32 1, i32 2, i32 3, i32 0>
// CHECK: ret void
