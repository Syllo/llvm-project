// REQUIRES: aarch64-registered-target
// RUN: %clang -g --target=aarch64-unknown-linux-gnu -S -O2 -emit-llvm -fenable-ripple -Xclang -verify %s

// Checks that we get an error for externf2/externf3 usage (no ripple_vector_candidate peer),
// and that externf1 (which has one) is accepted.

#include "../ripple_test.h"

typedef float v4f32 __attribute__((__vector_size__(16))) __attribute__((aligned(16)));

extern float externf1(ripple_block_t BS);
extern float externf2(ripple_block_t BS);
extern float externf3(ripple_block_t BS, float);

extern __attribute__((ripple_vector_candidate)) v4f32 ripple_externf1(void);

void test_invalid_candidates(float *restrict f, float *restrict f2, float *restrict f3) { // expected-error {{Ripple failed to vectorize this function}}
  ripple_block_t BS = ripple_set_block_shape(0, 4, 4);
  size_t RID = ripple_id(BS, 0);
  size_t RID2 = ripple_id(BS, 1);

  f[RID] = externf1(BS);
  f[RID2] += externf2(BS); // expected-error {{Passing a ripple block shape to a function call with no known definition is not allowed. Make sure that the function is available for ripple processing.}}
}

void test_invalid_candidates_2(float *restrict f, float *restrict f2, float *restrict f3) { // expected-error {{Ripple failed to vectorize this function}}
  ripple_block_t BS = ripple_set_block_shape(0, 4, 4);
  size_t RID = ripple_id(BS, 0);
  size_t RID2 = ripple_id(BS, 1);

  f[RID] = externf1(BS);
  f[RID2] += externf3(BS, 42.f); // expected-error {{Passing a ripple block shape to a function call with no known definition is not allowed. Make sure that the function is available for ripple processing.}}
}
