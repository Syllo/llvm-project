// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// REQUIRES: rt-lib
// RUN: %clang -g -S -fenable-ripple --target=hexagon -mhvx -mv85 -emit-llvm -mllvm -print-after=ripple %s -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s

#include "ripple_test.h"
#include <ripple_math.h>

#define N 1024
#define VECTOR_LANE 0

#define gen_div_test(N, LONGTYPE, SHORTTYPE)  \
  void div_kernel_##SHORTTYPE(LONGTYPE a[N], LONGTYPE b[N], LONGTYPE c[N]) { \
    ripple_block_t BS = ripple_set_block_shape(0, 33); \
    size_t v = ripple_id(BS, 0); \
    c[v] = a[v] / b[v]; \
  }


gen_div_test(64, uint16_t, u16)
// CHECK: call <32 x i32> @ripple_ew_pure_divu
// CHECK: call <32 x i32> @ripple_ew_pure_divu

gen_div_test(64, int16_t, i16)
// CHECK: call <32 x i32> @ripple_ew_pure_divi
// CHECK: call <32 x i32> @ripple_ew_pure_divi

gen_div_test(32, uint32_t, u32)
// CHECK: call <32 x i32> @ripple_ew_pure_divu
// CHECK: call <32 x i32> @ripple_ew_pure_divu

gen_div_test(32, int32_t, i32)
// CHECK: call <32 x i32> @ripple_ew_pure_divi
// CHECK: call <32 x i32> @ripple_ew_pure_divi

#if __has_Float16__
gen_div_test(64, _Float16, f16)
// CHECK: call <64 x half> @ripple_ew_pure_divf16
#endif

gen_div_test(32, float, f32)
// CHECK: call <32 x float> @ripple_ew_pure_divf
// CHECK: call <32 x float> @ripple_ew_pure_divf
