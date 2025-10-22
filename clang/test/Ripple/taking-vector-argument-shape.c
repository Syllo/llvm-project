// REQUIRES: hexagon-registered-target
// RUN: %clang -O2 --target=hexagon-unknown-elf -fenable-ripple -emit-llvm -S -o - %s | FileCheck %s


#include <ripple_hvx.h>

// CHECK-LABEL: @test
// CHECK-SAME: <128 x i32> noundef %[[V0:[0-9]+]], <128 x i8> noundef %[[SRC:[0-9]+]]
// CHECK: %[[ID:[0-9]+]] = bitcast <128 x i8> %[[SRC]] to <32 x i32>
// CHECK-NEXT: tail call void @ff(<32 x i32> noundef %[[ID]])

void ff(v32i32 x);

void test(v128i32 index, v128i8 src) {
  ripple_block_t BS = ripple_set_block_shape(0, 64, 2);
  int32_t idx = hvx_to_ripple_2d(BS, 128, i32, index);
  ff(src);
}