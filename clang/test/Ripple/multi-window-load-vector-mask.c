// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

#include "ripple_test.h"

#define VEC 0

// Sparse window lanes are physical byte offsets, while the vector branch mask
// is in logical tensor-lane order. Remap the three active logical lanes to
// physical lanes 0, 2, and 4 in each window.
// CHECK-LABEL: define{{.*}}@masked_multi_window_load_with_holes
// CHECK-NOT: llvm.masked.gather
// CHECK-COUNT-4: call <16 x i16> @llvm.masked.load.v16i16.p0({{.*}}<16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 false,
void masked_multi_window_load_with_holes(int16_t A[32][32], int16_t *Out) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
  size_t v0 = ripple_id(BS, 0);
  size_t v1 = ripple_id(BS, 1);

  if (v0 != 3)
    Out[v1 * 4 + v0] = A[v1][v0 * 2];
}
