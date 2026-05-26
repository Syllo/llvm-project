// REQUIRES: hexagon-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang -O2 -fenable-ripple --target=hexagon -mhvx -mv79 -mllvm -ripple-disable-link -emit-llvm -S %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -O2 -fenable-ripple --target=x86_64-unknown-linux-gnu -mllvm -ripple-disable-link -emit-llvm -S %s -o - | FileCheck %s %}

#include <ripple.h>

typedef float f32t32 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));

extern float myaddf(float a, float b);

__attribute__((ripple_vector_candidate)) extern f32t32
ripple_ew_pure_myaddf(f32t32 a, f32t32 b);

void compute(float *out, float *a, float *b) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  __SIZE_TYPE__ i = ripple_id(BS, 0);
  out[i] = myaddf(a[i], b[i]);
}

// CHECK-LABEL: define {{.*}} @compute(
// CHECK:       call {{.*}} @ripple_ew_pure_myaddf(
// CHECK-NOT:   call float @myaddf(
