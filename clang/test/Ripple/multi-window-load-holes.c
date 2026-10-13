// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i32 %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 %}

#include "ripple_test.h"

#define VEC 0

// Four windows each load offsets 0, 4, 8, and 12 relative to their base.
// CHECK-LABEL: define{{.*}}@multi_window_load_with_holes
// CHECK: [[LOAD0:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 %A, <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
// CHECK: [[BASE1:%[^ ]+]] = getelementptr i8, ptr %A, [[INDEX_TYPE]] 64
// CHECK: {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[BASE1]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
// CHECK: [[BASE2:%[^ ]+]] = getelementptr i8, ptr %A, [[INDEX_TYPE]] 128
// CHECK: {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[BASE2]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
// CHECK: [[BASE3:%[^ ]+]] = getelementptr i8, ptr %A, [[INDEX_TYPE]] 192
// CHECK: {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[BASE3]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
void multi_window_load_with_holes(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][v0 * 2];
}
