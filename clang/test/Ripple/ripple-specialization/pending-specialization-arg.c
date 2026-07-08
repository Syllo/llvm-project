// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=hexagon -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}
// RUN: %if x86-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=x86_64-unknown-linux-gnu -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}
// RUN: %if aarch64-registered-target %{ %clang -Wall -Wextra -Wpedantic -fenable-ripple --target=aarch64-unknown-linux-gnu -S -O2 -emit-llvm %s -o - | FileCheck --implicit-check-not="warning:" %s %}

#include "../ripple_test.h"

// Check pending specialization lowering does not lead to an error.

// CHECK-LABEL: define internal fastcc {{.*}} <32 x i32> @ripple.specialization.final{{.*}}.bar(
// CHECK-SAME: <32 x ptr> %[[PTR:[0-9]+]])
// CHECK: @llvm.masked.gather.v32i32.v32p0(<32 x ptr> align 4 %[[PTR]]
// CHECK: ret <32 x i32>

__attribute__((noinline)) int bar(int *p) { return *p + 1; }

void foo(int *in, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  out[v0] = bar(&in[v0]);
}
