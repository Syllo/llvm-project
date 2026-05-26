// Tests that static inline ripple_vector_candidate functions defined in headers
// are emitted into the IR by the clang frontend even when they have no direct
// call sites, so the Ripple pass can discover and register them as in-module
// candidates.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang++ -O2 -fenable-ripple --target=hexagon -mhvx -mv79 -Xclang -disable-llvm-passes -emit-llvm -S %s -o - | FileCheck %s --check-prefix=EMIT %}
// RUN: %if x86-registered-target %{ %clang++ -O2 -fenable-ripple --target=x86_64-unknown-linux-gnu -Xclang -disable-llvm-passes -emit-llvm -S %s -o - | FileCheck %s --check-prefix=EMIT %}
// RUN: %if aarch64-registered-target %{ %clang++ -O2 -fenable-ripple --target=aarch64-unknown-linux-gnu -Xclang -disable-llvm-passes -emit-llvm -S %s -o - | FileCheck %s --check-prefix=EMIT %}

#include <ripple.h>

typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));

// Scalar function called inside the ripple block. Declaration only — no body.
extern "C" float my_scalar_fn(float a, float b);

// Vector candidate defined as static inline in the same TU (simulating a
// header-only library). Must be emitted even though it is never called directly.
extern "C" {
static inline __attribute__((ripple_vector_candidate))
f32t32 ripple_ew_pure_my_scalar_fn(f32t32 a, f32t32 b) {
  f32t32 result;
  for (int i = 0; i < 32; ++i)
    result[i] = a[i] + b[i];
  return result;
}
}

// EMIT: define internal {{.*}} @_ZL{{[0-9]+}}ripple_ew_pure_my_scalar_fnDv32_fS_{{.*}} #[[ATTRS:[0-9]+]]
// EMIT: attributes #[[ATTRS]] = {{{.*}}"ripple-vector-candidate"{{.*}}}

void compute(float *out, float *a, float *b) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  __SIZE_TYPE__ i = ripple_id(BS, 0);
  out[i] = my_scalar_fn(a[i], b[i]);
}
