// REQUIRES: hexagon-registered-target
// RUN: %clang --target=hexagon-unknown-elf -g -c -O2 -fenable-ripple -emit-llvm %S/external_library.c -o %t.rlib.bc
// RUN: %clang --target=hexagon-unknown-elf -c -O2 -fenable-ripple -emit-llvm -S -o - -fripple-lib %t.rlib.bc -mllvm -ripple-disable-link %s | FileCheck %s

#include "external_library.h"
#include "../ripple_test.h"

#define VEC 0

// The 4x32 block touches 35 unique source elements (LS values 0..34) per
// iteration, so the window-load+shuffle path must (a) mask exactly 35 lanes
// active and (b) emit a shuffle whose indices are in load-element units —
// the unscaled LS pattern, *not* doubled byte offsets.
// CHECK-LABEL: define dso_local void @ew_multidim(
// CHECK: @llvm.masked.load.v128f16.p0(ptr align 2 %{{.*}}, <128 x i1> <i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>, <128 x half> poison)
// CHECK: shufflevector <128 x half> %{{.*}}, <128 x half> poison, <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 1, i32 2, i32 3, i32 4, i32 2, i32 3, i32 4, i32 5, i32 3, i32 4, i32 5, i32 6, i32 4, i32 5, i32 6, i32 7, i32 5, i32 6, i32 7, i32 8, i32 6, i32 7, i32 8, i32 9, i32 7, i32 8, i32 9, i32 10, i32 8, i32 9, i32 10, i32 11, i32 9, i32 10, i32 11, i32 12, i32 10, i32 11, i32 12, i32 13, i32 11, i32 12, i32 13, i32 14, i32 12, i32 13, i32 14, i32 15, i32 13, i32 14, i32 15, i32 16, i32 14, i32 15, i32 16, i32 17, i32 15, i32 16, i32 17, i32 18, i32 16, i32 17, i32 18, i32 19, i32 17, i32 18, i32 19, i32 20, i32 18, i32 19, i32 20, i32 21, i32 19, i32 20, i32 21, i32 22, i32 20, i32 21, i32 22, i32 23, i32 21, i32 22, i32 23, i32 24, i32 22, i32 23, i32 24, i32 25, i32 23, i32 24, i32 25, i32 26, i32 24, i32 25, i32 26, i32 27, i32 25, i32 26, i32 27, i32 28, i32 26, i32 27, i32 28, i32 29, i32 27, i32 28, i32 29, i32 30, i32 28, i32 29, i32 30, i32 31, i32 29, i32 30, i32 31, i32 32, i32 30, i32 31, i32 32, i32 33, i32 31, i32 32, i32 33, i32 34>
// CHECK: call void @ripple_ew_pure_cosf16(
// CHECK: @llvm.masked.scatter.v128f16.v128p0

void ew_multidim(size_t size, _Float16 *input, _Float16 *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 4, 32);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockY = ripple_id(BS, 1);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t BlockSizeY = ripple_get_block_size(BS, 1);
  size_t i;
  for (i = 0; i + BlockSizeX + BlockSizeY < size; i += BlockSizeX + BlockSizeY)
    output[i + BlockX + BlockY] = cosf16(input[i + BlockX + BlockY]);
  if (i + BlockX + BlockY < size)
    output[i + BlockX + BlockY] = cosf16(input[i + BlockX + BlockY]);
}
