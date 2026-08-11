// REQUIRES: hexagon-registered-target
// RUN: %clang %s -O2 -target hexagon -mv79 -mhvx -mhvx-length=128B -fenable-ripple -S -emit-llvm -o - | FileCheck %s

// Regression test for a Ripple::applyMaskToOps crash. ripple_to_vec /
// vec_to_ripple materialize a whole HVX vector via a per-lane store followed
// by a bulk vector load (or vice versa), so the resulting load/store has an
// LLVM vector type but a lane-uniform (Ripple-scalar) shape. applyMaskToOps
// used to dispatch on the LLVM type instead of the Ripple shape, so it built
// an i1 mask for what it treated as a vector op and crashed inside
// llvm.masked.load/store. It must dispatch on the Ripple shape instead.

#include "hexagon_types.h"
#include <ripple_hvx.h>

// CHECK: call void @llvm.masked.store.v32f32.p0(<32 x float> {{.*}}, ptr align 128 {{.*}}, <32 x i1> {{.*}})
void f(float a[32], int b[32]) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = a[ripple_id(BS, 0)];
  int y;
  if (x > 0.f) {
    y = vec_to_ripple<32, int32_t>(
        BS, Q6_Vw_equals_Vsf((ripple_to_vec<32, float>(BS, x))));
  } else {
    y = -x;
  }
  b[ripple_id(BS, 0)] = y;
}
