// REQUIRES: x86-registered-target || aarch64-registered-target || hexagon-registered-target
// RUN: %if x86-registered-target %{ %clang -target x86_64-unknown-linux-gnu -ffreestanding -S -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -target aarch64-unknown-linux-gnu -march=armv9.2-a -ffreestanding -S -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if hexagon-registered-target %{ %clang -target hexagon -mv81 -mhvx -ffreestanding -S -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

#include "ripple_test.h"

// Exercise native bf16 builtins directly, including on targets whose public
// math wrappers promote bf16 arguments to float.

// CHECK-LABEL: @scalar_sin(
// CHECK: call bfloat @llvm.sin.bf16(bfloat
__bf16 scalar_sin(__bf16 x) {
  return __builtin_ripple_sinbf16(x);
}

// CHECK-LABEL: @scalar_sqrt(
// CHECK: call bfloat @llvm.sqrt.bf16(bfloat
__bf16 scalar_sqrt(__bf16 x) {
  return __builtin_ripple_sqrtbf16(x);
}

// CHECK-LABEL: @scalar_pow(
// CHECK: call bfloat @llvm.pow.bf16(bfloat {{.*}}, bfloat
__bf16 scalar_pow(__bf16 x, __bf16 y) {
  return __builtin_ripple_powbf16(x, y);
}

// CHECK-LABEL: @scalar_ldexp(
// CHECK: call bfloat @llvm.ldexp.bf16.i32(bfloat {{.*}}, i32
__bf16 scalar_ldexp(__bf16 x, int exponent) {
  return __builtin_ripple_ldexpbf16(x, exponent);
}

// CHECK-LABEL: @scalar_sincos(
// CHECK: call { bfloat, bfloat } @llvm.sincos.bf16(bfloat
// CHECK: extractvalue { bfloat, bfloat } {{.*}}, 0
// CHECK: store bfloat
// CHECK: extractvalue { bfloat, bfloat } {{.*}}, 1
// CHECK: store bfloat
void scalar_sincos(__bf16 x, __bf16 *s, __bf16 *c) {
  __builtin_ripple_sincosbf16(x, s, c);
}

// CHECK-LABEL: @scalar_modf(
// CHECK: call { bfloat, bfloat } @llvm.modf.bf16(bfloat
// CHECK: extractvalue { bfloat, bfloat } {{.*}}, 0
// CHECK: store bfloat
// CHECK: extractvalue { bfloat, bfloat } {{.*}}, 1
__bf16 scalar_modf(__bf16 x, __bf16 *integral) {
  return __builtin_ripple_modfbf16(x, integral);
}

// CHECK-LABEL: @scalar_frexp(
// CHECK: call { bfloat, i32 } @llvm.frexp.bf16.i32(bfloat
// CHECK: extractvalue { bfloat, i32 } {{.*}}, 1
// CHECK: store i32
// CHECK: extractvalue { bfloat, i32 } {{.*}}, 0
__bf16 scalar_frexp(__bf16 x, int *exponent) {
  return __builtin_ripple_frexpbf16(x, exponent);
}

// CHECK-LABEL: @vector_sin(
// CHECK: call <16 x bfloat> @llvm.sin.v16bf16(<16 x bfloat>
void vector_sin(const __bf16 *x, __bf16 *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 16);
  int i = ripple_id(BS, 0);
  y[i] = __builtin_ripple_sinbf16(x[i]);
}

// CHECK-LABEL: @vector_sqrt(
// CHECK: call <16 x bfloat> @llvm.sqrt.v16bf16(<16 x bfloat>
void vector_sqrt(const __bf16 *x, __bf16 *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 16);
  int i = ripple_id(BS, 0);
  y[i] = __builtin_ripple_sqrtbf16(x[i]);
}

// CHECK-LABEL: @vector_pow(
// CHECK: call <16 x bfloat> @llvm.pow.v16bf16(<16 x bfloat> {{.*}}, <16 x bfloat>
void vector_pow(const __bf16 *x, const __bf16 *y, __bf16 *z) {
  ripple_block_t BS = ripple_set_block_shape(0, 16);
  int i = ripple_id(BS, 0);
  z[i] = __builtin_ripple_powbf16(x[i], y[i]);
}

// Candidate lookup must also recognize the bf16 spelling of math intrinsics.
typedef __bf16 bf16t8 __attribute__((vector_size(16)));
extern __attribute__((ripple_vector_candidate)) bf16t8
ripple_ew_pure_cosbf16(bf16t8);

// CHECK-LABEL: @vector_cos_candidate(
// CHECK: call{{.*}} @ripple_ew_pure_cosbf16(
void vector_cos_candidate(const __bf16 *x, __bf16 *y) {
  ripple_block_t BS = ripple_set_block_shape(0, 16);
  int i = ripple_id(BS, 0);
  y[i] = __builtin_ripple_cosbf16(x[i]);
}
