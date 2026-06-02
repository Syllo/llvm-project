// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -mhvx-qfloat -emit-llvm %s \
// RUN:   -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s
#include "ripple_test.h"
// HVX_MathLib RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
typedef _Float16 f16t64 __attribute__((vector_size(128)));
typedef float f32t32 __attribute__((vector_size(128)));
typedef int32_t i32t32 __attribute__((vector_size(128)));
typedef int16_t i16t64 __attribute__((vector_size(128)));
typedef uint32_t u32t32 __attribute__((vector_size(128)));
typedef uint16_t u16t64 __attribute__((vector_size(128)));
#define DECLARE_MATH_INTRINSIC_VARIANT_UNARY(INTRINSIC_NAME, FROM_SCALAR_TYPE, \
                                             FROM_TYPE, LIBTAG, SUFFIX)        \
  extern RIPPLE_VECTOR_CANDIDATE FROM_TYPE                                     \
  ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(FROM_TYPE input);

#define DECLARE_MATH_INTRINSIC_VARIANT_BINARY(                                 \
    INTRINSIC_NAME, FROM_SCALAR_TYPE, FROM_TYPE, LIBTAG, SUFFIX)               \
  extern RIPPLE_VECTOR_CANDIDATE FROM_TYPE                                     \
  ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(FROM_TYPE input1,  \
                                                            FROM_TYPE input2);

#ifdef __cplusplus
extern "C" {
#endif

float sqrtf_safe(float);
float sqrtf_fast(float);
float sqrtf_fast_safe(float);

_Float16 sqrtf16_safe(_Float16);
_Float16 sqrtf16_fast(_Float16);
_Float16 sqrtf16_fast_safe(_Float16);

float rsqrtf(float);
float rsqrtf_safe(float);
float rsqrtf_fast(float);
float rsqrtf_fast_safe(float);

_Float16 rsqrtf16(_Float16);
_Float16 rsqrtf16_safe(_Float16);
_Float16 rsqrtf16_fast(_Float16);
_Float16 rsqrtf16_fast_safe(_Float16);

float hypotf(float, float);
float hypotf_safe(float, float);
float hypotf_fast(float, float);
float hypotf_fast_safe(float, float);

_Float16 hypotf16(_Float16, _Float16);
_Float16 hypotf16_safe(_Float16, _Float16);
_Float16 hypotf16_fast(_Float16, _Float16);
_Float16 hypotf16_fast_safe(_Float16, _Float16);

float tanhf_fast(float);

float expf(float);

float expf_fast(float x);

float expf_fast_UNSAFE(float x);

_Float16 expf16(_Float16);

_Float16 expf16_fast(_Float16 x);

_Float16 expf16_fast_UNSAFE(_Float16 x);

float logf(float);

float logf_fast(float x);

_Float16 logf16(_Float16);

_Float16 logf16_fast(_Float16 x);

float log2f(float);

float log2f_fast(float x);

_Float16 log2f16(_Float16);

_Float16 log2f16_fast(_Float16 x);

float log10f(float);

float log10f_fast(float x);

_Float16 log10f16(_Float16);

_Float16 log10f16_fast(_Float16 x);

_Float16 sinhf16(_Float16);

_Float16 sinhf16_fast(_Float16 x);

_Float16 coshf16(_Float16);

_Float16 coshf16_fast(_Float16 x);

float divf(float x, float y);
float recipf(float x);

float divf_fast(float x, float y);
float recipf_fast(float x);

_Float16 divf16(_Float16 x, _Float16 y);
_Float16 recipf16(_Float16 x);

_Float16 divf16_fast(_Float16 x, _Float16 y);
_Float16 recipf16_fast(_Float16 x);

int32_t divi(int32_t x, int32_t y);
uint32_t divu(uint32_t x, uint32_t y);
int32_t remi(int32_t x, int32_t y);
uint32_t remu(uint32_t x, uint32_t y);

int16_t divi16(int16_t x, int16_t y);
uint16_t divu16(uint16_t x, uint16_t y);
int16_t remi16(int16_t x, int16_t y);
uint16_t remu16(uint16_t x, uint16_t y);

_Float16 powf16_fast(_Float16, _Float16);

_Float16 tanf16_fast(_Float16);
_Float16 cotf16_fast(_Float16);

#if __has_bf16__
#if __has_soft_bf16__
#define wrap_spec_unary_bf16_mathfn(op)                                        \
  __attribute__((always_inline)) static __bf16 op##bf16(const __bf16 Val) {    \
    return (__bf16)op##f((float)Val);                                          \
  }

#define wrap_spec_binary_bf16_mathfn(op)                                       \
  __attribute__((always_inline)) static __bf16 op##bf16(const __bf16 X,        \
                                                        const __bf16 Y) {      \
    return (__bf16)op##f((float)(X), (float)(Y));                              \
  }

wrap_spec_binary_bf16_mathfn(hypot);
wrap_spec_unary_bf16_mathfn(rsqrt);
#endif
#endif

DECLARE_MATH_INTRINSIC_VARIANT_UNARY(acos, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(acos, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(asin, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(asin, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(atan, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(atan, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cos, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cos, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cosh, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cosh, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cosh, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(cot, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(ceil, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(ceil, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, i, i32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, u, u32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, i16, i16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(div, u16, u16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f, f32t32, ew_pure_, _fast_UNSAFE)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(exp, f16, f16t64, ew_pure_, _fast_UNSAFE)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(floor, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(floor, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f, f32t32, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f, f32t32, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f16, f16t64, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(hypot, f16, f16t64, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log2, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log2, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log2, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log2, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log10, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log10, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log10, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(log10, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(pow, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(pow, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(pow, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(recip, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(recip, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(recip, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(recip, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(rem, i, i32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(rem, u, u32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(rem, i16, i16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_BINARY(rem, u16, u16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(round, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(round, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f, f32t32, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f, f32t32, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f16, f16t64, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(rsqrt, f16, f16t64, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sin, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sin, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sinh, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sinh, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sinh, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f, f32t32, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f, f32t32, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f16, f16t64, ew_pure_, _safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(sqrt, f16, f16t64, ew_pure_, _fast_safe)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tan, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tan, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tan, f16, f16t64, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tanh, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tanh, f, f32t32, ew_pure_, _fast)
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(tanh, f16, f16t64, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(trunc, f, f32t32, ew_pure_, )
DECLARE_MATH_INTRINSIC_VARIANT_UNARY(trunc, f16, f16t64, ew_pure_, )

#ifdef __cplusplus
}
#endif

#undef DECLARE_MATH_INTRINSIC_VARIANT_UNARY
#undef DECLARE_MATH_INTRINSIC_VARIANT_BINARY
#include <ripple_math.h>

#define gen_check_mathfn_replacement_unary(N, VEC, OP, LONGTYPE)               \
  void check_HVX_##OP(const LONGTYPE x[N], LONGTYPE y[N]) {                    \
    ripple_block_t BS = ripple_set_block_shape(0, VEC);                                             \
    ripple_parallel(BS, 0);                                                     \
    for (int i = 0; i < N; i++) {                                              \
      y[i] = OP(x[i]);                                                 \
    }                                                                          \
  }

#define gen_check_mathfn_replacement_binary(N, VEC, OP, LONGTYPE)              \
  void check_HVX_##OP(const LONGTYPE x1[N], LONGTYPE x2[N], LONGTYPE y[N]) {   \
    ripple_block_t BS = ripple_set_block_shape(0, VEC);                                             \
    ripple_parallel(BS, 0);                                                     \
    for (int i = 0; i < N; i++) {                                              \
      y[i] = OP(x1[i], x2[i]);                                                 \
    }                                                                          \
  }

gen_check_mathfn_replacement_unary(32, 32, acosf, float);
// CHECK: @check_HVX_acosf
// CHECK: call <32 x float> @ripple_ew_pure_acosf
// ret void

gen_check_mathfn_replacement_unary(64, 64, acosf16, _Float16);
// CHECK: @check_HVX_acosf16
// CHECK: call <64 x half> @ripple_ew_pure_acosf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, asinf, float);
// CHECK: @check_HVX_asinf
// CHECK: call <32 x float> @ripple_ew_pure_asinf
// ret void

gen_check_mathfn_replacement_unary(64, 64, asinf16, _Float16);
// CHECK: @check_HVX_asinf16
// CHECK: call <64 x half> @ripple_ew_pure_asinf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, atanf, float);
// CHECK: @check_HVX_atanf
// CHECK: call <32 x float> @ripple_ew_pure_atanf
// ret void

gen_check_mathfn_replacement_unary(64, 64, atanf16, _Float16);
// CHECK: @check_HVX_atanf16
// CHECK: call <64 x half> @ripple_ew_pure_atanf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, cosf, float);
// CHECK: @check_HVX_cosf
// CHECK: call <32 x float> @ripple_ew_pure_cosf
// ret void

gen_check_mathfn_replacement_unary(64, 64, cosf16, _Float16);
// CHECK: @check_HVX_cosf16
// CHECK: call <64 x half> @ripple_ew_pure_cosf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, coshf, float);
// CHECK: @check_HVX_coshf
// CHECK: call <32 x float> @ripple_ew_pure_coshf
// ret void

gen_check_mathfn_replacement_unary(64, 64, coshf16, _Float16);
// CHECK: @check_HVX_coshf16
// CHECK: call <64 x half> @ripple_ew_pure_coshf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, coshf16_fast, _Float16);
// CHECK: @check_HVX_coshf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_coshf16_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, cotf16_fast, _Float16);
// CHECK: @check_HVX_cotf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_cotf16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, sinf, float);
// CHECK: @check_HVX_sinf
// CHECK: call <32 x float> @ripple_ew_pure_sinf
// ret void

gen_check_mathfn_replacement_unary(64, 64, sinf16, _Float16);
// CHECK: @check_HVX_sinf16
// CHECK: call <64 x half> @ripple_ew_pure_sinf16
// ret void

gen_check_mathfn_replacement_binary(32, 32, divf, float);
// CHECK: @check_HVX_divf
// CHECK: call <32 x float> @ripple_ew_pure_divf
// ret void

gen_check_mathfn_replacement_binary(32, 32, divf_fast, float);
// CHECK: @check_HVX_divf_fast
// CHECK: call <32 x float> @ripple_ew_pure_divf_fast
// ret void

gen_check_mathfn_replacement_binary(64, 64, divf16_fast, _Float16);
// CHECK: @check_HVX_divf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_divf16_fast
// ret void

gen_check_mathfn_replacement_binary(32, 32, divi, int32_t);
// CHECK: @check_HVX_divi
// CHECK: call <32 x i32> @ripple_ew_pure_divi
// ret void

gen_check_mathfn_replacement_binary(32, 32, divu, uint32_t);
// CHECK: @check_HVX_divu
// CHECK: call <32 x i32> @ripple_ew_pure_divu
// ret void

gen_check_mathfn_replacement_binary(64, 64, divi16, int16_t);
// CHECK: @check_HVX_divi16
// CHECK: call <64 x i16> @ripple_ew_pure_divi16
// ret void

gen_check_mathfn_replacement_binary(64, 64, divu16, uint16_t);
// CHECK: @check_HVX_divu16
// CHECK: call <64 x i16> @ripple_ew_pure_divu16
// ret void

gen_check_mathfn_replacement_unary(32, 32, expf, float);
// CHECK: @check_HVX_expf
// CHECK: call <32 x float> @ripple_ew_pure_expf
// ret void

gen_check_mathfn_replacement_unary(32, 32, expf_fast, float);
// CHECK: @check_HVX_expf_fast
// CHECK: call <32 x float> @ripple_ew_pure_expf_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, expf_fast_UNSAFE, float);
// CHECK: @check_HVX_expf_fast_UNSAFE
// CHECK: call <32 x float> @ripple_ew_pure_expf_fast_UNSAFE
// ret void

gen_check_mathfn_replacement_unary(64, 64, expf16, _Float16);
// CHECK: @check_HVX_expf16
// CHECK: call <64 x half> @ripple_ew_pure_expf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, expf16_fast, _Float16);
// CHECK: @check_HVX_expf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_expf16_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, expf16_fast_UNSAFE, _Float16);
// CHECK: @check_HVX_expf16_fast_UNSAFE
// CHECK: call <64 x half> @ripple_ew_pure_expf16_fast_UNSAFE
// ret void

gen_check_mathfn_replacement_binary(32, 32, hypotf, float);
// CHECK: @check_HVX_hypotf
// CHECK: call <32 x float> @ripple_ew_pure_hypotf
// ret void

gen_check_mathfn_replacement_binary(32, 32, hypotf_safe, float);
// CHECK: @check_HVX_hypotf_safe
// CHECK: call <32 x float> @ripple_ew_pure_hypotf_safe
// ret void

gen_check_mathfn_replacement_binary(32, 32, hypotf_fast, float);
// CHECK: @check_HVX_hypotf_fast
// CHECK: call <32 x float> @ripple_ew_pure_hypotf_fast
// ret void

gen_check_mathfn_replacement_binary(32, 32, hypotf_fast_safe, float);
// CHECK: @check_HVX_hypotf_fast_safe
// CHECK: call <32 x float> @ripple_ew_pure_hypotf_fast_safe
// ret void

gen_check_mathfn_replacement_binary(64, 64, hypotf16, _Float16);
// CHECK: @check_HVX_hypotf16
// CHECK: call <64 x half> @ripple_ew_pure_hypotf16
// ret void

gen_check_mathfn_replacement_binary(64, 64, hypotf16_safe, _Float16);
// CHECK: @check_HVX_hypotf16_safe
// CHECK: call <64 x half> @ripple_ew_pure_hypotf16_safe
// ret void

gen_check_mathfn_replacement_binary(64, 64, hypotf16_fast, _Float16);
// CHECK: @check_HVX_hypotf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_hypotf16_fast
// ret void

gen_check_mathfn_replacement_binary(64, 64, hypotf16_fast_safe, _Float16);
// CHECK: @check_HVX_hypotf16_fast_safe
// CHECK: call <64 x half> @ripple_ew_pure_hypotf16_fast_safe
// ret void

#if __has_bf16__
gen_check_mathfn_replacement_binary(64, 64, hypotbf16, __bf16);
// CHECK: @check_HVX_hypotbf16
// CHECK: call <32 x float> @ripple_ew_pure_hypotf
// CHECK: call <32 x float> @ripple_ew_pure_hypotf
// ret void
#endif

gen_check_mathfn_replacement_unary(32, 32, logf, float);
// CHECK: @check_HVX_logf
// CHECK: call <32 x float> @ripple_ew_pure_logf
// ret void

gen_check_mathfn_replacement_unary(64, 64, logf16, _Float16);
// CHECK: @check_HVX_logf16
// CHECK: call <64 x half> @ripple_ew_pure_logf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, logf_fast, float);
// CHECK: @check_HVX_logf_fast
// CHECK: call <32 x float> @ripple_ew_pure_logf_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, logf16_fast, _Float16);
// CHECK: @check_HVX_logf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_logf16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, log2f, float);
// CHECK: @check_HVX_log2f
// CHECK: call <32 x float> @ripple_ew_pure_log2f
// ret void

gen_check_mathfn_replacement_unary(64, 64, log2f16, _Float16);
// CHECK: @check_HVX_log2f16
// CHECK: call <64 x half> @ripple_ew_pure_log2f16
// ret void

gen_check_mathfn_replacement_unary(32, 32, log2f_fast, float);
// CHECK: @check_HVX_log2f_fast
// CHECK: call <32 x float> @ripple_ew_pure_log2f_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, log2f16_fast, _Float16);
// CHECK: @check_HVX_log2f16_fast
// CHECK: call <64 x half> @ripple_ew_pure_log2f16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, log10f, float);
// CHECK: @check_HVX_log10f
// CHECK: call <32 x float> @ripple_ew_pure_log10f
// ret void

gen_check_mathfn_replacement_unary(64, 64, log10f16, _Float16);
// CHECK: @check_HVX_log10f16
// CHECK: call <64 x half> @ripple_ew_pure_log10f16
// ret void

gen_check_mathfn_replacement_unary(32, 32, log10f_fast, float);
// CHECK: @check_HVX_log10f_fast
// CHECK: call <32 x float> @ripple_ew_pure_log10f_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, log10f16_fast, _Float16);
// CHECK: @check_HVX_log10f16_fast
// CHECK: call <64 x half> @ripple_ew_pure_log10f16_fast
// ret void

gen_check_mathfn_replacement_binary(32, 32, powf, float);
// CHECK: @check_HVX_powf
// CHECK: call <32 x float> @ripple_ew_pure_powf
// ret void

gen_check_mathfn_replacement_binary(64, 64, powf16, _Float16);
// CHECK: @check_HVX_powf16
// CHECK: call <64 x half> @ripple_ew_pure_powf16
// ret void

gen_check_mathfn_replacement_binary(64, 64, powf16_fast, _Float16);
// CHECK: @check_HVX_powf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_powf16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, rsqrtf, float);
// CHECK: @check_HVX_rsqrtf
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf
// ret void

gen_check_mathfn_replacement_unary(32, 32, rsqrtf_safe, float);
// CHECK: @check_HVX_rsqrtf_safe
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf_safe
// ret void

gen_check_mathfn_replacement_unary(32, 32, rsqrtf_fast, float);
// CHECK: @check_HVX_rsqrtf_fast
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, rsqrtf_fast_safe, float);
// CHECK: @check_HVX_rsqrtf_fast_safe
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf_fast_safe
// ret void

gen_check_mathfn_replacement_unary(64, 64, rsqrtf16, _Float16);
// CHECK: @check_HVX_rsqrtf16
// CHECK: call <64 x half> @ripple_ew_pure_rsqrtf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, rsqrtf16_safe, _Float16);
// CHECK: @check_HVX_rsqrtf16_safe
// CHECK: call <64 x half> @ripple_ew_pure_rsqrtf16_safe
// ret void

gen_check_mathfn_replacement_unary(64, 64, rsqrtf16_fast, _Float16);
// CHECK: @check_HVX_rsqrtf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_rsqrtf16_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, rsqrtf16_fast_safe, _Float16);
// CHECK: @check_HVX_rsqrtf16_fast_safe
// CHECK: call <64 x half> @ripple_ew_pure_rsqrtf16_fast_safe
// ret void

#if __has_bf16__
gen_check_mathfn_replacement_unary(64, 64, rsqrtbf16, __bf16);
// CHECK: @check_HVX_rsqrtbf16
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf
// CHECK: call <32 x float> @ripple_ew_pure_rsqrtf
// ret void
#endif

gen_check_mathfn_replacement_unary(32, 32, sinhf, float);
// CHECK: @check_HVX_sinhf
// CHECK: call <32 x float> @ripple_ew_pure_sinhf
// ret void

gen_check_mathfn_replacement_unary(64, 64, sinhf16, _Float16);
// CHECK: @check_HVX_sinhf16
// CHECK: call <64 x half> @ripple_ew_pure_sinhf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, sinhf16_fast, _Float16);
// CHECK: @check_HVX_sinhf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_sinhf16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, sqrtf, float);
// CHECK: @check_HVX_sqrtf
// CHECK: call <32 x float> @ripple_ew_pure_sqrtf
// ret void

gen_check_mathfn_replacement_unary(32, 32, sqrtf_safe, float);
// CHECK: @check_HVX_sqrtf_safe
// CHECK: call <32 x float> @ripple_ew_pure_sqrtf_safe
// ret void

gen_check_mathfn_replacement_unary(32, 32, sqrtf_fast, float);
// CHECK: @check_HVX_sqrtf_fast
// CHECK: call <32 x float> @ripple_ew_pure_sqrtf_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, sqrtf_fast_safe, float);
// CHECK: @check_HVX_sqrtf_fast_safe
// CHECK: call <32 x float> @ripple_ew_pure_sqrtf_fast_safe
// ret void

gen_check_mathfn_replacement_unary(64, 64, sqrtf16, _Float16);
// CHECK: @check_HVX_sqrtf16
// CHECK: call <64 x half> @ripple_ew_pure_sqrtf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, sqrtf16_safe, _Float16);
// CHECK: @check_HVX_sqrtf16_safe
// CHECK: call <64 x half> @ripple_ew_pure_sqrtf16_safe
// ret void

gen_check_mathfn_replacement_unary(64, 64, sqrtf16_fast, _Float16);
// CHECK: @check_HVX_sqrtf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_sqrtf16_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, sqrtf16_fast_safe, _Float16);
// CHECK: @check_HVX_sqrtf16_fast_safe
// CHECK: call <64 x half> @ripple_ew_pure_sqrtf16_fast_safe
// ret void

gen_check_mathfn_replacement_unary(32, 32, tanf, float);
// CHECK: @check_HVX_tanf
// CHECK: call <32 x float> @ripple_ew_pure_tanf
// ret void

gen_check_mathfn_replacement_unary(64, 64, tanf16, _Float16);
// CHECK: @check_HVX_tanf16
// CHECK: call <64 x half> @ripple_ew_pure_tanf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, tanf16_fast, _Float16);
// CHECK: @check_HVX_tanf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_tanf16_fast
// ret void

gen_check_mathfn_replacement_unary(32, 32, tanhf, float);
// CHECK: @check_HVX_tanhf
// CHECK: call <32 x float> @ripple_ew_pure_tanhf
// ret void

gen_check_mathfn_replacement_unary(32, 32, tanhf_fast, float);
// CHECK: @check_HVX_tanhf_fast
// CHECK: call <32 x float> @ripple_ew_pure_tanhf_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, tanhf16, _Float16);
// CHECK: @check_HVX_tanhf16
// CHECK: call <64 x half> @ripple_ew_pure_tanhf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, truncf, float);
// CHECK: @check_HVX_truncf
// CHECK: call <32 x float> @ripple_ew_pure_truncf
// ret void

gen_check_mathfn_replacement_unary(64, 64, truncf16, _Float16);
// CHECK: @check_HVX_truncf16
// CHECK: call <64 x half> @ripple_ew_pure_truncf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, roundf, float);
// CHECK: @check_HVX_roundf
// CHECK: call <32 x float> @ripple_ew_pure_roundf
// ret void

gen_check_mathfn_replacement_unary(64, 64, roundf16, _Float16);
// CHECK: @check_HVX_roundf16
// CHECK: call <64 x half> @ripple_ew_pure_roundf16

gen_check_mathfn_replacement_unary(32, 32, ceilf, float);
// CHECK: @check_HVX_ceilf
// CHECK: call <32 x float> @ripple_ew_pure_ceilf
// ret void

gen_check_mathfn_replacement_unary(64, 64, ceilf16, _Float16);
// CHECK: @check_HVX_ceilf16
// CHECK: call <64 x half> @ripple_ew_pure_ceilf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, floorf, float);
// CHECK: @check_HVX_floorf
// CHECK: call <32 x float> @ripple_ew_pure_floorf
// ret void

gen_check_mathfn_replacement_unary(64, 64, floorf16, _Float16);
// CHECK: @check_HVX_floorf16
// CHECK: call <64 x half> @ripple_ew_pure_floorf16
// ret void

gen_check_mathfn_replacement_unary(32, 32, recipf, float);
// CHECK: @check_HVX_recipf
// CHECK: call <32 x float> @ripple_ew_pure_recipf
// ret void

gen_check_mathfn_replacement_unary(32, 32, recipf_fast, float);
// CHECK: @check_HVX_recipf_fast
// CHECK: call <32 x float> @ripple_ew_pure_recipf_fast
// ret void

gen_check_mathfn_replacement_unary(64, 64, recipf16, _Float16);
// CHECK: @check_HVX_recipf16
// CHECK: call <64 x half> @ripple_ew_pure_recipf16
// ret void

gen_check_mathfn_replacement_unary(64, 64, recipf16_fast, _Float16);
// CHECK: @check_HVX_recipf16_fast
// CHECK: call <64 x half> @ripple_ew_pure_recipf16_fast
// ret void

gen_check_mathfn_replacement_binary(32, 32, remi, int32_t);
// CHECK: @check_HVX_remi
// CHECK: call <32 x i32> @ripple_ew_pure_remi
// ret void

gen_check_mathfn_replacement_binary(32, 32, remu, uint32_t);
// CHECK: @check_HVX_remu
// CHECK: call <32 x i32> @ripple_ew_pure_remu
// ret void

gen_check_mathfn_replacement_binary(64, 64, remi16, int16_t);
// CHECK: @check_HVX_remi16
// CHECK: call <64 x i16> @ripple_ew_pure_remi16
// ret void

gen_check_mathfn_replacement_binary(64, 64, remu16, uint16_t);
// CHECK: @check_HVX_remu16
// CHECK: call <64 x i16> @ripple_ew_pure_remu16
// ret void
