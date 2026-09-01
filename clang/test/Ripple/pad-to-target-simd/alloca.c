// REQUIRES: hexagon-registered-target
// RUN: %clang -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O0 \
// RUN:     -fenable-ripple -fdisable-ripple-lib \
// RUN:     -mllvm -ripple-pad-to-target-simd -emit-llvm %s -o - 2>&1 \
// RUN:     | FileCheck %s --check-prefix=HVX

// Keep this at O0: Ripple expands the scalar local into an alloca of an array
// with the block shape. PadToTargetSIMD must turn that array into a legal HVX
// vector length instead of abandoning the complete padding transformation.

#include "../ripple_test.h"

void pad_ripple_alloca(const float *in, float *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 3);
  size_t i = ripple_id(BS, 0);
  float local = in[i];
  out[i] = local + 1.0f;
}

// With HVX width selected, v3 is padded to the native v32 width.
// HVX-LABEL: define {{.*}}@pad_ripple_alloca
// HVX: alloca [32 x float]
// HVX-NOT: Not padding Ripple generated vectors
