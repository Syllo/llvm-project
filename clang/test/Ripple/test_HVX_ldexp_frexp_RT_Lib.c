// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang -g -S -fenable-ripple --target=hexagon -O2 -mhvx -mv81 -emit-llvm %s \
// RUN: -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s

// This test verifies that the Ripple compiler pass replaces calls to the
// ldexpf/frexpf (and _Float16 variants) math builtins with the corresponding
// ripple_ew_pure_* runtime implementations from HVX_ldexp_frexp.h.
//
// The runtime declarations are inlined here so this frontend test does not
// depend on the compiler-rt headers or the Hexagon SDK.
//
#include "ripple_test.h"

#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));
typedef int32_t i32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));
typedef _Float16 f16t64 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));
typedef int32_t i32t64 __attribute__((__vector_size__(256)))
    __attribute__((aligned(128)));

extern float ldexpf(float, int);
extern _Float16 ldexpf16(_Float16, int);
extern float frexpf(float, int *);
extern _Float16 frexpf16(_Float16, int *);

extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_pure_ldexpf(f32t32, i32t32);
extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_pure_ldexpf16(f16t64, i32t64);
extern RIPPLE_VECTOR_CANDIDATE f32t32
ripple_ret_t32f32_t32f32_t32ptr_frexpf(f32t32, i32t32 *);
extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_ret_t64f16_t64f16_t64ptr_frexpf16(f16t64, i32t64 *);

void check_HVX_ldexpf(const float x[32], const int32_t exp[32],
                       float y[32]) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  ripple_parallel(BS, 0);
  for (int i = 0; i < 32; i++)
    y[i] = ldexpf(x[i], exp[i]);
}
// CHECK-LABEL: @check_HVX_ldexpf
// CHECK: call <32 x float> @ripple_ew_pure_ldexpf

void check_HVX_ldexpf16(const _Float16 x[64], const int32_t exp[64],
                         _Float16 y[64]) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  ripple_parallel(BS, 0);
  for (int i = 0; i < 64; i++)
    y[i] = ldexpf16(x[i], exp[i]);
}
// CHECK-LABEL: @check_HVX_ldexpf16
// CHECK: call <64 x half> @ripple_ew_pure_ldexpf16

void check_HVX_frexpf(const float x[32], float y[32], int32_t exp[32]) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  size_t v0 = ripple_id(BS, 0);
  int exp_tmp = ripple_broadcast(BS, 0b1, 0);
  y[v0] = frexpf(x[v0], &exp_tmp);
  exp[v0] = exp_tmp;
}
// CHECK-LABEL: @check_HVX_frexpf
// CHECK: call <32 x float> @ripple_ret_t32f32_t32f32_t32ptr_frexpf

void check_HVX_frexpf16(const _Float16 x[64], _Float16 y[64],
                         int32_t exp[64]) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  size_t v0 = ripple_id(BS, 0);
  int exp_tmp = ripple_broadcast(BS, 0b1, 0);
  y[v0] = frexpf16(x[v0], &exp_tmp);
  exp[v0] = exp_tmp;
}
// CHECK-LABEL: @check_HVX_frexpf16
// CHECK: call <64 x half> @ripple_ret_t64f16_t64f16_t64ptr_frexpf16
