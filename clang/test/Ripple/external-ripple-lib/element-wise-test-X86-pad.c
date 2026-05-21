// REQUIRES: x86-registered-target
// RUN: %clang --target=x86_64-unknown-elf -mavx2 -c -O2 -fenable-ripple -emit-llvm -S -o - -mllvm -ripple-pad-to-target-simd %s | FileCheck %s

#include "../ripple_test.h"

typedef float f32t32 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));

extern float doublify(float);
extern __attribute__((ripple_vector_candidate)) f32t32 ripple_ew_doublify(f32t32);

void foo(const float A[37], float EightA[37]) {
  ripple_block_t BS = ripple_set_block_shape(0, 37);
  size_t v0 = ripple_id(BS, 0);
  EightA[v0] = doublify(4 * A[v0]);
}
// CHECK-LABEL: foo
// CHECK: call <40 x float> @llvm.masked.load.v40f32.p0
// CHECK: fmul <40 x float>
// CHECK: call <32 x float> @ripple_ew_doublify(ptr nonnull
// CHECK: call <32 x float> @ripple_ew_doublify(ptr nonnull
// CHECK: call void @llvm.masked.store.v40f32.p0
// CHECK: ret void
