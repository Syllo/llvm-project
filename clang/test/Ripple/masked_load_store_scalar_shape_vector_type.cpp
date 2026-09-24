// REQUIRES: hexagon-registered-target
// RUN: %clang %s -O2 -target hexagon -mv79 -mhvx -mhvx-length=128B -fenable-ripple -S -emit-llvm -o - | FileCheck %s

// Regression test for a Ripple::applyMaskToOps crash. The old ripple_to_vec /
// vec_to_ripple implementation materialized whole HVX vectors through
// scalar-shaped vector loads and stores. Fixed-vector boundaries now lower
// those conversions directly, without introducing masked memory operations.

#include "hexagon_types.h"
#include <ripple_hvx.h>

// CHECK-NOT: llvm.masked
// CHECK: [[BITS:%.*]] = bitcast <32 x float> {{.*}} to <32 x i32>
// CHECK: [[RESULT:%.*]] = {{(tail )?}}call <32 x i32> @llvm.hexagon.V6.vconv.w.sf.128B(<32 x i32> [[BITS]])
// CHECK: [[MERGED:%.*]] = select <32 x i1> {{.*}}, <32 x i32> [[RESULT]], <32 x i32> {{.*}}
// CHECK: store <32 x i32> [[MERGED]], ptr %b
void f(float a[32], int b[32]) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = a[ripple_id(BS, 0)];
  int y;
  if (x > 0.f) {
    y = vec_to_ripple<int32_t>(
        BS, Q6_Vw_equals_Vsf((ripple_to_vec<32, float>(x))));
  } else {
    y = -x;
  }
  b[ripple_id(BS, 0)] = y;
}
