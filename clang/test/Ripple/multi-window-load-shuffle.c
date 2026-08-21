// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i32 --check-prefixes=CHECK,HEXAGON,ROW-UNFOLDED %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 --check-prefixes=CHECK,FOLDED,ROW-UNFOLDED %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s -DINDEX_TYPE=i64 --check-prefixes=CHECK,FOLDED,AARCH64 %}

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
// CHECK: [[ROW_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, [[INDEX_TYPE]] 64
// CHECK: [[ROW_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE1]]
//
// Place the second window in result lanes 4-7.
// ROW-UNFOLDED: [[ROW_POS1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD1]], <16 x i16> poison, <16 x i32> <i32 poison, i32 poison, i32 poison, i32 poison, i32 0, i32 1, i32 2, i32 3
//
// Merge lanes 0-3 from the first window with lanes 4-7 from the second.
// Indices 20-23 mean lanes 4-7 of the second shuffle operand.
// ROW-UNFOLDED: [[ROW_MERGE1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD0]], <16 x i16> [[ROW_POS1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 20, i32 21, i32 22, i32 23
// AArch64 folds the lane placement into this merge.
// AARCH64: [[ROW_MERGE1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD0]], <16 x i16> [[ROW_LOAD1]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 16, i32 17, i32 18, i32 19, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
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

// Row-major 4x4 tile load with holes within each load window.
//
// The accessed byte offsets are:
//
//   0,4,8,12,
//   64,68,72,76,
//   128,132,136,140,
//   192,196,200,204
//
// Each row fits within a single vector-sized load window, but the requested
// elements are non-contiguous. Ripple should form four windows and use a
// sparse mask to load only the requested elements.
//
// CHECK-LABEL: define{{.*}}@multi_window_load_with_holes
// First window loads offsets [0, 4, 8, 12].
// CHECK: [[ROW_LOAD0:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 %A, <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// Second window starts at byte offset 64.
// CHECK: [[ROW_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, i64 64
// Second window loads offsets [64, 68, 72, 76].
// CHECK: [[ROW_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE1]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// In the final result, merge lane 0-3 using local lanes from 0,2,4 and 6 from first window
// Similarly, merge lane 4-7 in the result using local lanes 0,2,4 and 6 from second window
// CHECK: [[ROW_MERGE1_2:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD0]], <16 x i16> [[ROW_LOAD1]], <16 x i32> <i32 0, i32 2, i32 4, i32 6, i32 16, i32 18, i32 20, i32 22, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
//
// Remaining two window loads.
// CHECK-COUNT-2: call <16 x i16> @llvm.masked.load.v16i16.p0
//
// CHECK: store <16 x i16>
void multi_window_load_with_holes(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][v0 * 2];
}

// Row-major 4x4 tile load with holes within each load window.
//
// The accessed byte offsets are:
//
// 12, 8, 4, 0,
// 76, 72, 68, 64,
// 140, 136, 132, 128,
// 204, 200, 196, 192
//
// CHECK-LABEL: define{{.*}}@multi_window_load_with_holes_unsorted
// First window loads offsets Before sorting [12, 8, 4, 0].
// After sorting - [0, 4, 8, 12]
// CHECK: [[ROW_LOAD0:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 %A, <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// Second window starts at byte offset 64.
// CHECK: [[ROW_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, i64 64
// Second window loads offsets before sorting [76, 72, 68, 64].
// CHECK: [[ROW_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE1]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// Reorder the first two loaded windows into their original lane order
// CHECK: [[SHUFFLE1:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD0]], <16 x i16> [[ROW_LOAD1]], <16 x i32> <i32 6, i32 4, i32 2, i32 0, i32 22, i32 20, i32 18, i32 16, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>
//
// Third window starts at byte offset 128.
// CHECK: [[ROW_BASE2:%[^ ]+]] = getelementptr i8, ptr %A, i64 128
// Third window loads offsets before sorting [140, 136, 132, 128].
// CHECK: [[ROW_LOAD2:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE2]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// Place offsets [140, 136, 132, 128] into result lanes 8-11.
// CHECK: [[SHUFFLE2:%[^ ]+]] = shufflevector <16 x i16> [[ROW_LOAD2]], <16 x i16> poison, <16 x i32> <i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 6, i32 4, i32 2, i32 0, i32 poison, i32 poison, i32 poison, i32 poison>
//
// Merge the third window with the first two windows.
// Indices 24-27 select lanes 8-11 from the second shuffle operand.
// CHECK: [[MERGE2:%[^ ]+]] = shufflevector <16 x i16> [[SHUFFLE1]], <16 x i16> [[SHUFFLE2]], <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 24, i32 25, i32 26, i32 27, i32 poison, i32 poison, i32 poison, i32 poison>
//
// Similarly load and merge window 4
// Fourth window starts at byte offset 192.
// CHECK: [[ROW_BASE3:%[^ ]+]] = getelementptr i8, ptr %A, i64 192
// Fourth window loads offsets before sorting [204, 200, 196, 192].
// CHECK: [[ROW_LOAD3:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[ROW_BASE3]], <16 x i1> <i1 true, i1 false, i1 true, i1 false, i1 true, i1 false, i1 true, i1 false,
//
// CHECK: store <16 x i16>
void multi_window_load_with_holes_unsorted(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][6 - 2 * v0];
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
// HEXAGON: [[COL_POS0:%[^ ]+]] = shufflevector <16 x i16> [[COL_LOAD0]], <16 x i16> poison, <16 x i32> <i32 0, i32 poison, i32 poison, i32 poison, i32 1, i32 poison, i32 poison, i32 poison, i32 2, i32 poison, i32 poison, i32 poison, i32 3, i32 poison, i32 poison, i32 poison>
// CHECK: [[COL_BASE1:%[^ ]+]] = getelementptr i8, ptr %A, [[INDEX_TYPE]] 64
// CHECK: [[COL_LOAD1:%[^ ]+]] = {{.*}}call <16 x i16> @llvm.masked.load.v16i16.p0(ptr align 2 [[COL_BASE1]]
//
// Window 0 contributes offsets [0,2,4,6] to result lanes [0,4,8,12].
// Window 1 contributes offsets [64,66,68,70] to result lanes [1,5,9,13].
//
// X86 and AArch64 fold lane placement into the merge: indices 0-3 select
// COL_LOAD0 and indices 16-19 select COL_LOAD1. Hexagon retains the two
// placement shuffles and merges their corresponding lanes instead.
// FOLDED: [[COL_MERGE01:%[^ ]+]] = shufflevector <16 x i16> [[COL_LOAD0]], <16 x i16> [[COL_LOAD1]], <16 x i32> <i32 0, i32 16, i32 poison, i32 poison, i32 1, i32 17, i32 poison, i32 poison, i32 2, i32 18, i32 poison, i32 poison, i32 3, i32 19, i32 poison, i32 poison>
// HEXAGON: [[COL_POS1:%[^ ]+]] = shufflevector <16 x i16> [[COL_LOAD1]], <16 x i16> poison, <16 x i32> <i32 poison, i32 0, i32 poison, i32 poison, i32 poison, i32 1, i32 poison, i32 poison, i32 poison, i32 2, i32 poison, i32 poison, i32 poison, i32 3, i32 poison, i32 poison>
// HEXAGON: [[COL_MERGE01:%[^ ]+]] = shufflevector <16 x i16> [[COL_POS0]], <16 x i16> [[COL_POS1]], <16 x i32> <i32 0, i32 17, i32 poison, i32 poison, i32 4, i32 21, i32 poison, i32 poison, i32 8, i32 25, i32 poison, i32 poison, i32 12, i32 29, i32 poison, i32 poison>
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
// CHECK: getelementptr i8, ptr %A, <16 x [[INDEX_TYPE]]> <[[INDEX_TYPE]] 0, [[INDEX_TYPE]] 64, [[INDEX_TYPE]] 128, [[INDEX_TYPE]] 192, [[INDEX_TYPE]] 2048, [[INDEX_TYPE]] 2112, [[INDEX_TYPE]] 2176, [[INDEX_TYPE]] 2240, [[INDEX_TYPE]] 4096, [[INDEX_TYPE]] 4160, [[INDEX_TYPE]] 4224, [[INDEX_TYPE]] 4288, [[INDEX_TYPE]] 6144, [[INDEX_TYPE]] 6208, [[INDEX_TYPE]] 6272, [[INDEX_TYPE]] 6336>
// CHECK: llvm.masked.gather
void no_multi_window_load(int16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 32 + v0] = A[v1 * 32 + v0][0];
}
