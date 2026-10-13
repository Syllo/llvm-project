// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i32 %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -fno-discard-value-names -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 %}

#include "ripple_test.h"

#define VEC 0

// Sorting accesses [12, 8, 4, 0] creates a negative offset relative to the
// pre-offset GEP base. The resulting windows must still be based at A + 0,
// A + 64, A + 128, and A + 192 and restore the descending lane order.
// CHECK-LABEL: define{{.*}}@multi_window_load_with_holes_unsorted
// CHECK-DAG: {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 %A, <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
// CHECK-DAG: getelementptr i8, ptr %A, [[INDEX_TYPE]] 64
// CHECK-DAG: getelementptr i8, ptr %A, [[INDEX_TYPE]] 128
// CHECK-DAG: getelementptr i8, ptr %A, [[INDEX_TYPE]] 192
// CHECK-DAG: shufflevector <16 x i16> {{.*}}<16 x i32> <i32 6, i32 4, i32 2, i32 0,
void multi_window_load_with_holes_unsorted(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][6 - 2 * v0];
}
