// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=hexagon -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}
// RUN: %if x86-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=x86_64-unknown-linux-gnu -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}
// RUN: %if aarch64-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=aarch64-unknown-linux-gnu -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}

#include "../ripple_test.h"

// Check pending specialization lowering supports stores through specialized
// pointer arguments.

// CHECK-LABEL: define internal fastcc void @ripple.specialization.final{{.*}}.bar(
// CHECK-SAME: <32 x ptr> %[[PTR:[0-9]+]], <32 x i32> %[[VAL:[0-9]+]])
// CHECK: %[[ADD:.*]] = add <32 x i32> %[[VAL]], splat (i32 1)
// CHECK: @llvm.masked.scatter.v32i32.v32p0(<32 x i32> %[[ADD]], <32 x ptr> align 4 %[[PTR]]
// CHECK: ret void

__attribute__((noinline)) void bar(int *p, int x) { *p = x + 1; }

void foo(int *in, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  bar(&out[v0], in[v0]);
}
