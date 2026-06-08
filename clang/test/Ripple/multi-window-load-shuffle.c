// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}

#include "ripple_test.h"

#define VEC 0

// Row-major 4x4 tile load from a 32x32 matrix.
//
// The accessed addresses are:
//
//   0, 2, 4, 6,
//   64, 66, 68, 70,
//   128,130,132,134,
//   192,194,196,198
//
// These addresses are not fully contiguous, but they naturally form four
// contiguous windows, one per matrix row. Ripple should recognize this and
// replace the masked gather with four coalesced vector loads.
//
// The loaded values are already in the desired lane order, so no final
// transpose/shuffle is required.
//
// First window loads offsets [0, 2, 4, 6].
// CHECK: [[ROW_LOAD0:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0
//
// Second window starts at byte offset 64.
// CHECK: [[ROW_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, i64 64
// CHECK: [[ROW_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE1]]
//
// Place the second window in result lanes 4-7.
// CHECK: [[ROW_POS1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD1]], <16 x i16> poison, <16 x i32> <i32 poison, i32 poison, i32 poison, i32 poison, i32 0, i32 1, i32 2, i32 3
//
// Merge lanes 0-3 from the first window with lanes 4-7 from the second.
// Indices 20-23 mean lanes 4-7 of the second shuffle operand.
// CHECK: [[ROW_MERGE1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD0]], <16 x i16> [[ROW_POS1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 20, i32 21, i32 22, i32 23
//
// Remaining two window loads.
// CHECK-COUNT-2: call <16 x i16> @llvm.masked.load.v16i16.p0
//
// CHECK: store <16 x i16>
void multi_window_load_row_major(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][v0];
}

// Column-major (transposed) 4x4 tile load from a 32x32 matrix.
//
// The accessed addresses are:
//
//   0,64,128,192,
//   2,66,130,194,
//   4,68,132,196,
//   6,70,134,198
//
// The addresses still fall into the same four contiguous row windows:
//
//   [0,2,4,6]
//   [64,66,68,70]
//   [128,130,132,134]
//   [192,194,196,198]
//
// so Ripple should replace the masked gather with four coalesced vector
// loads. Unlike the row-major case, the loaded values must be rearranged
// into the requested transposed lane order, requiring a final shuffle.
//
// Expected transpose mask:
//
//   <0,4,8,12,
//    1,5,9,13,
//    2,6,10,14,
//    3,7,11,15>
//
// CHECK-LABEL: define{{.*}}@multi_window_load_shuffle_col_major
// CHECK-NOT: llvm.masked.gather
// CHECK: [[COL_LOAD0:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0
// CHECK: [[COL_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, i64 64
// CHECK: [[COL_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[COL_BASE1]]
//
// Window 0 contributes offsets [0,2,4,6] to result lanes [0,4,8,12].
// Window 1 contributes offsets [64,66,68,70] to result lanes [1,5,9,13].
//
// Indices 0-3 select lanes from COL_LOAD0.
// Indices 16-19 select lanes 0-3 from COL_LOAD1.
// CHECK: [[COL_MERGE01:%[^ ]+]] = shufflevector <16 x i16> [[COL_LOAD0]], <16 x i16> [[COL_LOAD1]], <16 x i32> <i32 0, i32 16, i32 poison, i32 poison, i32 1, i32 17, i32 poison, i32 poison, i32 2, i32 18, i32 poison, i32 poison, i32 3, i32 19, i32 poison, i32 poison>
//
// CHECK-COUNT-2: call <16 x i16> @llvm.masked.load.v16i16.p0
//
// CHECK: store <16 x i16>
void multi_window_load_shuffle_col_major(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v0][v1];
}

// All windows are are of size = 1, fallback to
// llvm.masked.gather
// CHECK-LABEL: define{{.*}}@no_multi_window_load
// CHECK: getelementptr i8, ptr %A, <16 x i64> <i64 0, i64 64, i64 128, i64 192, i64 2048, i64 2112, i64 2176, i64 2240, i64 4096, i64 4160, i64 4224, i64 4288, i64 6144, i64 6208, i64 6272, i64 6336>
// CHECK: llvm.masked.gather
void no_multi_window_load(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 32 + v0] = A[v1 * 32 + v0][0];
}
