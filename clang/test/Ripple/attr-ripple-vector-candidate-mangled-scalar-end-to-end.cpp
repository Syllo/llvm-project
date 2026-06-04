// End-to-end check that the ripple pass demangles a C++ mangled scalar call
// before matching it against an in-module ripple_vector_candidate. The scalar
// myaddf is declared with C++ linkage, so its call site is emitted as the
// Itanium symbol @_Z6myaddfff; the pass must reduce that to the source base
// name "myaddf" to select the candidate ripple_ew_pure_myaddf.
//
// The <32 x float> candidate is selected on targets whose vectorization width
// accommodates it (Hexagon HVX and x86); aarch64's narrower native SIMD makes
// the pass serialize instead, so it is not exercised here.
// REQUIRES: hexagon-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang++ -O2 -fenable-ripple --target=hexagon -mhvx -mv79 -emit-llvm -S %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang++ -O2 -fenable-ripple --target=x86_64-unknown-linux-gnu -emit-llvm -S %s -o - | FileCheck %s %}

#include <ripple.h>

typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));

// C++ scalar function: its call site is mangled to @_Z6myaddfff.
float myaddf(float a, float b);

// Candidate exposed as a C-linkage symbol whose source base name is "myaddf".
extern "C" __attribute__((ripple_vector_candidate))
f32t32 ripple_ew_pure_myaddf(f32t32 a, f32t32 b);

void compute(float *out, float *a, float *b) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  __SIZE_TYPE__ i = ripple_id(BS, 0);
  out[i] = myaddf(a[i], b[i]);
}

// CHECK-LABEL: define {{.*}} @_Z7computePfS_S_(
// CHECK:         call {{.*}} @ripple_ew_pure_myaddf(
// CHECK-NOT:     call float @_Z6myaddfff(
