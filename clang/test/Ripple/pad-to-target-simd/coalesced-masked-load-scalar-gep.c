// REQUIRES: hexagon-registered-target
// RUN: %clang -ffreestanding -S --target=hexagon-unknown-elf -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -mllvm -ripple-pad-to-target-simd -emit-llvm %s -o - | FileCheck %s

#include "../ripple_test.h"

// A coalesced sparse load needs a scalar byte-GEP for its second window.  The
// padding pass must retain that GEP's scalar width.
// CHECK-LABEL: define dso_local void @coalesced_masked_load_scalar_gep(
// CHECK:         getelementptr i8, ptr [[A:%.*]], i32 240
// CHECK:         call <128 x i16> @llvm.masked.load.v128i16.p0(
// CHECK-SAME:      ptr align 2 {{%.*}},
// CHECK:         ret void
void coalesced_masked_load_scalar_gep(int16_t *a, int16_t *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 120);
  size_t v0 = ripple_id(BS, 0);
  out[v0] = a[v0 * 2];
}
