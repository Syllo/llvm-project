// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - | FileCheck %s %}

#include "ripple_test.h"

#define VEC 0

// The PE access pattern below is overlapping (the outer block dim is not
// multiplied by the inner dim size), so the addresses fit in a small window
// with duplicates. This exercises the masked-load + permute path in
// genWindowLoadShuffle: the load width matches the tensor shape (128),
// but the actual access window is only Max-Min+1 lanes wide, so the
// trailing lanes are masked off to avoid faulting on memory the original
// gather would never have touched.
// CHECK-LABEL: define{{.*}}@window_load_shuffle_2d
// CHECK: tail call <128 x i16> @llvm.masked.load.v128i16.p0(ptr align 2 %{{.*}}, <128 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, <128 x i16> poison)
// CHECK: shufflevector <128 x i16> %{{.*}}, <128 x i16> poison, <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 1, i32 2, i32 3, i32 4, i32 2, i32 3, i32 4, i32 5, i32 3, i32 4, i32 5, i32 6, i32 4, i32 5, i32 6, i32 7, i32 5, i32 6, i32 7, i32 8, i32 6, i32 7, i32 8, i32 9, i32 7, i32 8, i32 9, i32 10, i32 8, i32 9, i32 10, i32 11, i32 9, i32 10, i32 11, i32 12, i32 10, i32 11, i32 12, i32 13, i32 11, i32 12, i32 13, i32 14, i32 12, i32 13, i32 14, i32 15, i32 13, i32 14, i32 15, i32 16, i32 14, i32 15, i32 16, i32 17, i32 15, i32 16, i32 17, i32 18, i32 16, i32 17, i32 18, i32 19, i32 17, i32 18, i32 19, i32 20, i32 18, i32 19, i32 20, i32 21, i32 19, i32 20, i32 21, i32 22, i32 20, i32 21, i32 22, i32 23, i32 21, i32 22, i32 23, i32 24, i32 22, i32 23, i32 24, i32 25, i32 23, i32 24, i32 25, i32 26, i32 24, i32 25, i32 26, i32 27, i32 25, i32 26, i32 27, i32 28, i32 26, i32 27, i32 28, i32 29, i32 27, i32 28, i32 29, i32 30, i32 28, i32 29, i32 30, i32 31, i32 29, i32 30, i32 31, i32 32, i32 30, i32 31, i32 32, i32 33, i32 31, i32 32, i32 33, i32 34>

void window_load_shuffle_2d(size_t size, int16_t *input, int16_t *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 4, 32);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockY = ripple_id(BS, 1);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t BlockSizeY = ripple_get_block_size(BS, 1);
  for (size_t i = 0; i < size; i += BlockSizeX + BlockSizeY)
    output[i + BlockX + BlockY] = input[i + BlockX + BlockY] + (int16_t)1;
}

// Same overlapping access pattern, but the trailing `if` clause forces
// Ripple to if-convert the tail and apply a runtime mask via
// applyMaskToOps. The window load must keep its NumElementsToLoad-wide
// shape so that mask propagation matches the load's lane count.
// CHECK-LABEL: define{{.*}}@window_load_shuffle_2d_masked
// CHECK: tail call <128 x i16> @llvm.masked.load.v128i16.p0(ptr align 2 %{{.*}}, <128 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, <128 x i16> poison)
// CHECK: shufflevector <128 x i16> %{{.*}}, <128 x i16> poison, <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 1, i32 2, i32 3, i32 4, i32 2, i32 3, i32 4, i32 5, i32 3, i32 4, i32 5, i32 6, i32 4, i32 5, i32 6, i32 7, i32 5, i32 6, i32 7, i32 8, i32 6, i32 7, i32 8, i32 9, i32 7, i32 8, i32 9, i32 10, i32 8, i32 9, i32 10, i32 11, i32 9, i32 10, i32 11, i32 12, i32 10, i32 11, i32 12, i32 13, i32 11, i32 12, i32 13, i32 14, i32 12, i32 13, i32 14, i32 15, i32 13, i32 14, i32 15, i32 16, i32 14, i32 15, i32 16, i32 17, i32 15, i32 16, i32 17, i32 18, i32 16, i32 17, i32 18, i32 19, i32 17, i32 18, i32 19, i32 20, i32 18, i32 19, i32 20, i32 21, i32 19, i32 20, i32 21, i32 22, i32 20, i32 21, i32 22, i32 23, i32 21, i32 22, i32 23, i32 24, i32 22, i32 23, i32 24, i32 25, i32 23, i32 24, i32 25, i32 26, i32 24, i32 25, i32 26, i32 27, i32 25, i32 26, i32 27, i32 28, i32 26, i32 27, i32 28, i32 29, i32 27, i32 28, i32 29, i32 30, i32 28, i32 29, i32 30, i32 31, i32 29, i32 30, i32 31, i32 32, i32 30, i32 31, i32 32, i32 33, i32 31, i32 32, i32 33, i32 34>

void window_load_shuffle_2d_masked(size_t size, int16_t *input,
                                   int16_t *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 4, 32);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockY = ripple_id(BS, 1);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t BlockSizeY = ripple_get_block_size(BS, 1);
  size_t i;
  for (i = 0; i + BlockSizeX + BlockSizeY < size; i += BlockSizeX + BlockSizeY)
    output[i + BlockX + BlockY] = input[i + BlockX + BlockY] + (int16_t)1;
  if (i + BlockX + BlockY < size)
    output[i + BlockX + BlockY] = input[i + BlockX + BlockY] + (int16_t)1;
}

// 1D variant: a non-injective index expression (`v0 / 2`) collapses 8 PEs
// onto a 4-element window. The load's lane count (8) is wider than the
// window, so genWindowLoadShuffle emits a masked load whose first
// `WindowSize` lanes are active, followed by a shuffle that picks the
// right element per PE.
// CHECK-LABEL: define{{.*}}@window_load_shuffle_1d
// CHECK: tail call <8 x i16> @llvm.masked.load.v8i16.p0(
// CHECK-SAME: <8 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false>
// CHECK: shufflevector <8 x i16> %{{.*}}, <8 x i16> poison, <8 x i32> <i32 0, i32 0, i32 1, i32 1, i32 2, i32 2, i32 3, i32 3>

void window_load_shuffle_1d(int16_t *input, int16_t *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 8);
  size_t v0 = ripple_id(BS, 0);
  output[v0] = input[v0 / 2] + (int16_t)1;
}

// 1D plain-load variant: XOR-1 permutation of [0..7] is injective and
// WindowSize == NumElements, so genWindowLoadShuffle uses a plain vector
// load (no mask) followed by a shuffle.
// CHECK-LABEL: define{{.*}}@window_load_shuffle_1d_plain
// CHECK: load <8 x i16>, ptr
// CHECK: shufflevector <8 x i16> %{{.*}}, <8 x i16> poison, <8 x i32> <i32 1, i32 0, i32 3, i32 2, i32 5, i32 4, i32 7, i32 6>

void window_load_shuffle_1d_plain(int16_t *input, int16_t *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 8);
  size_t v0 = ripple_id(BS, 0);
  output[v0] = input[v0 ^ 1] + (int16_t)1;
}

