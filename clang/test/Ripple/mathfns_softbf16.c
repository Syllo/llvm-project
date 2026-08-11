// REQUIRES: hexagon-registered-target
// RUN: %clang -O2 -S -target hexagon -mhvx -mv81 -fenable-ripple -emit-llvm %s -o - 2>&1 | FileCheck %s

#include "ripple_test.h"
// Use fp32 builtins to test software bf16 math without registering HVX candidates.

void check_mylog(const float x[128], float y[128]) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS, 0);
  y[v0] = __builtin_ripple_logf(x[v0]);
  // CHECK: @check_mylog
  // CHECK: <128 x float> @llvm.log.v128f32(<128 x float> %{{[0-9]+}})
  // CHECK: ret
}

#define gen_check_unary_mathfn(N, OP, LONGTYPE, BUILTIN)                       \
  void check_##OP(const LONGTYPE x[N], LONGTYPE y[N]) {                        \
    ripple_block_t BS = ripple_set_block_shape(0, N);                          \
    int v0 = ripple_id(BS, 0);                                                 \
    y[v0] = BUILTIN(x[v0]);                                                    \
  }

gen_check_unary_mathfn(64, sqrtbf16, __bf16, __builtin_ripple_sqrtf);
// CHECK: @check_sqrtbf16
// CHECK: <64 x float> @llvm.sqrt.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, sinbf16, __bf16, __builtin_ripple_sinf);
// CHECK: @check_sinbf16
// CHECK: <64 x float> @llvm.sin.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, cosbf16, __bf16, __builtin_ripple_cosf);
// CHECK: @check_cosbf16
// CHECK: <64 x float> @llvm.cos.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, expbf16, __bf16, __builtin_ripple_expf);
// CHECK: @check_expbf16
// CHECK: <64 x float> @llvm.exp.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, exp2bf16, __bf16, __builtin_ripple_exp2f);
// CHECK: @check_exp2bf16
// CHECK: <64 x float> @llvm.exp2.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, exp10bf16, __bf16, __builtin_ripple_exp10f);
// CHECK: @check_exp10bf16
// CHECK: <64 x float> @llvm.exp10.v64f32(<64 x float> %{{.+}})
// CHECK: ret

gen_check_unary_mathfn(64, logbf16, __bf16, __builtin_ripple_logf);
// CHECK: @check_logbf16
// CHECK: <64 x float> @llvm.log.v64f32(<64 x float> %{{.+}})
// CHECK: ret
