// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %clang -S -fenable-ripple -emit-llvm -O2 %s -o - | FileCheck %s

// InstCombine fuses a sin(x) and cos(x) call sharing the same argument into a
// single aggregate-returning @llvm.sincos call (see foldSinAndCosToSinCos in
// InstCombineCalls.cpp). Ripple cannot create a vector type from an
// aggregate-returning instruction, so RippleAlgebraicTransformsPreprocessPass
// must undo this fusion before Ripple's vectorizer runs. Note that the final
// InstCombine run in the post-processing pipeline may re-fuse the vectorized
// sin/cos calls into @llvm.sincos again, so check that the functions vectorize
// successfully without requiring the final IR to keep sin/cos separate.

#include <ripple.h>
#include "ripple_test.h"
#include <ripple_math.h>

void foo(const float *x, float *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  y[v0] = sinf(x[v0]) + cosf(x[v0]);
}
// CHECK-LABEL: define {{.*}}@foo(
// CHECK: store <32 x float>
// CHECK: ret

__attribute__((noinline)) float sincos_specialized(float x) {
  return sinf(x) + cosf(x);
}
// CHECK-LABEL: define {{.*}}float @sincos_specialized(
// CHECK: ret float

// The scalar callee is cloned during shape propagation, after the module's
// initial preprocessing. Its specialization must also have sincos split.
void call_specialized(const float *x, float *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  y[v0] = sincos_specialized(x[v0]);
}
// CHECK-LABEL: define {{.*}}@call_specialized(
// CHECK: call {{.*}}<32 x float> @{{.*}}sincos_specialized
// CHECK: store <32 x float>
// CHECK: ret

// Shape propagation initially waits for the callee's specialization. Revisit
// this function to split sincos once the call's return shape is known.
void pending_specialization(const float *x, float *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  float v = sincos_specialized(x[v0]);
  y[v0] = sinf(v) + cosf(v);
}
// CHECK-LABEL: define {{.*}}@pending_specialization(
// CHECK: store <32 x float>
// CHECK: ret
