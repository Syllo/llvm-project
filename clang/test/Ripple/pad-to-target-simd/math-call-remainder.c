// Test that padToTargetSIMDWidth correctly handles the case where a
// ShuffleVectorInst (padding a <16 x float> remainder to <32 x float>) is
// replaced by a new instruction, and a CallInst that uses the padded result
// is NOT in InstsThatMustBePadded (because its argument is already at the
// target SIMD width).  Before the fix, the pass asserted
// "Replaced instruction still has uses" when trying to delete the
// shufflevector.
//
// The pattern arises when:
//   - N=400, CHUNK=32 => REM=16 (remainder loop has 16 iterations)
//   - Ripple vectorizes the remainder to <16 x float>
//   - padToTargetSIMDWidth pads <16 x float> -> <32 x float>
//   - The padded shufflevector feeds a ripple_ew_pure_acosf call
//   - The call is not in InstsThatMustBePadded (its arg is already 32-wide)
//
// REQUIRES: hexagon-registered-target
// RUN: %clang -S --target=hexagon-unknown-elf -mhvx -mv79 -mhvx-length=128B \
// RUN:     -O2 -fenable-ripple -fdisable-ripple-lib \
// RUN:     -mllvm -ripple-pad-to-target-simd \
// RUN:     -emit-llvm %s -o - 2>&1 | FileCheck %s

// CHECK-NOT: Assertion
// CHECK-NOT: has non empty uses
// CHECK: define

#include "../ripple_test.h"
#include <ripple_math.h>

#define N 400

float A[N];
float RES[N];

// This function has N=400, CHUNK=32, REM=16.  The remainder loop body calls
// acosf on a <16 x float> Ripple tensor.  After padding to <32 x float>,
// the shufflevector result feeds ripple_ew_pure_acosf which must not be
// left with dangling uses when the shufflevector is deleted.
void run(void) {
    float * __restrict__ pA = A;
    float * __restrict__ pRES = RES;

    #define CHUNK 32
    #define REM (N % CHUNK)
    for (int base = 0; base < (N - REM); base += CHUNK) {
        ripple_block_t BS = ripple_set_block_shape(0, CHUNK);
        ripple_parallel(BS, 0);
        for (int i = base; i < base + CHUNK; i++) {
            pRES[i] = acosf(pA[i]);
        }
    }
    if (REM > 0) {
        ripple_block_t BS = ripple_set_block_shape(0, REM);
        ripple_parallel(BS, 0);
        for (int i = N - REM; i < N; i++) {
            pRES[i] = acosf(pA[i]);
        }
    }
    #undef CHUNK
    #undef REM
}
