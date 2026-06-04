// End-to-end check that the ripple pass demangles an Itanium internal-linkage
// symbol (_ZL...ripple_ew_pure_...) so a static inline ripple_vector_candidate
// defined in the same TU is discovered and used to replace the scalar call.
//
// The <32 x float> candidate fits on targets whose vectorization width
// accommodates it (Hexagon HVX and x86); aarch64's narrower native SIMD makes
// the pass serialize instead, so it is not exercised here.
// REQUIRES: hexagon-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang++ -O2 -fenable-ripple --target=hexagon -mhvx -mv79 -emit-llvm -S %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang++ -O2 -fenable-ripple --target=x86_64-unknown-linux-gnu -emit-llvm -S %s -o - | FileCheck %s %}

#include <ripple.h>

typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));

// Scalar function called inside the ripple block. Declaration only — no body.
extern "C" float my_scalar_fn(float a, float b);

// Vector candidate defined as static inline in the same TU (simulating a
// header-only library). Its LLVM symbol is an Itanium internal name
// (_ZL...ripple_ew_pure_my_scalar_fn...); the pass must demangle it to
// discover the ripple_ prefix and the matching scalar name.
extern "C" {
static inline __attribute__((ripple_vector_candidate))
f32t32 ripple_ew_pure_my_scalar_fn(f32t32 a, f32t32 b) {
  f32t32 result;
  for (int i = 0; i < 32; ++i)
    result[i] = a[i] + b[i];
  return result;
}
}

void compute(float *out, float *a, float *b) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  __SIZE_TYPE__ i = ripple_id(BS, 0);
  out[i] = my_scalar_fn(a[i], b[i]);
}

// The ripple pass replaces the scalar my_scalar_fn call with the vector
// candidate; the candidate is then inlined into compute. Either way, no scalar
// call to my_scalar_fn must remain.
// CHECK-LABEL: define {{.*}} @_Z7computePfS_S_(
// CHECK-NOT:     call float @my_scalar_fn(
