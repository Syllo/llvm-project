//===---------------------------- HVX_Mathlib.h -----------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Header to access the standard math library (and some).
//===------------------------------------------------------------------------===

#pragma once
#include "HVX_ldexp_frexp.h"
#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <ripple.h>

#define RIPPLE_HVX_MATH_EXPAND(NAME) NAME

#ifdef __cplusplus
#define DEFINE_MATH_INTRINSIC_VARIANT_UNARY(ATTRIB, INTRINSIC_NAME,            \
                                            FROM_SCALAR_TYPE, FROM_TYPE,       \
                                            TO_TYPE, LIBTAG, SUFFIX)           \
  ATTRIB FROM_TYPE ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(  \
      FROM_TYPE input) {                                                       \
    static_assert(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                        \
                  "Incompatible type sizes");                                  \
    TO_TYPE CAST_INP;                                                          \
    FROM_TYPE CAST_OUT;                                                        \
    __builtin_memcpy(&CAST_INP, &input, sizeof(FROM_TYPE));                    \
    TO_TYPE result = RIPPLE_HVX_MATH_EXPAND(                                   \
        INTRINSIC_NAME##_v##FROM_SCALAR_TYPE##SUFFIX)(CAST_INP);               \
    __builtin_memcpy(&CAST_OUT, &result, sizeof(TO_TYPE));                     \
    return CAST_OUT;                                                           \
  }
#define DEFINE_MATH_INTRINSIC_VARIANT_BINARY(ATTRIB, INTRINSIC_NAME,           \
                                             FROM_SCALAR_TYPE, FROM_TYPE,      \
                                             TO_TYPE, LIBTAG, SUFFIX)          \
  ATTRIB FROM_TYPE ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(  \
      FROM_TYPE input1, FROM_TYPE input2) {                                    \
    static_assert(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                        \
                  "Incompatible type sizes");                                  \
    TO_TYPE CAST_INP1, CAST_INP2;                                              \
    FROM_TYPE CAST_OUT;                                                        \
    __builtin_memcpy(&CAST_INP1, &input1, sizeof(FROM_TYPE));                  \
    __builtin_memcpy(&CAST_INP2, &input2, sizeof(FROM_TYPE));                  \
    TO_TYPE result = RIPPLE_HVX_MATH_EXPAND(                                   \
        INTRINSIC_NAME##_v##FROM_SCALAR_TYPE##SUFFIX)(CAST_INP1, CAST_INP2);   \
    __builtin_memcpy(&CAST_OUT, &result, sizeof(TO_TYPE));                     \
    return CAST_OUT;                                                           \
  }
#else
typedef union {
  i32t32 as_i32t32;
  u32t32 as_u32t32;
  i16t64 as_i16t64;
  u16t64 as_u16t64;
  f32t32 as_f32t32;
  f16t64 as_f16t64;
} __RippleHVXMathLibCastUnion;
#define DEFINE_MATH_INTRINSIC_VARIANT_UNARY(ATTRIB, INTRINSIC_NAME,            \
                                            FROM_SCALAR_TYPE, FROM_TYPE,       \
                                            TO_TYPE, LIBTAG, SUFFIX)           \
  ATTRIB FROM_TYPE ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(  \
      FROM_TYPE input) {                                                       \
    static_assert(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                        \
                  "Incompatible type sizes");                                  \
    __RippleHVXMathLibCastUnion u;                                             \
    u.as_##FROM_TYPE = input;                                                  \
    TO_TYPE result = RIPPLE_HVX_MATH_EXPAND(                                   \
        INTRINSIC_NAME##_v##FROM_SCALAR_TYPE##SUFFIX)(u.as_##TO_TYPE);         \
    u.as_##TO_TYPE = result;                                                   \
    return u.as_##FROM_TYPE;                                                   \
  }
#define DEFINE_MATH_INTRINSIC_VARIANT_BINARY(ATTRIB, INTRINSIC_NAME,           \
                                             FROM_SCALAR_TYPE, FROM_TYPE,      \
                                             TO_TYPE, LIBTAG, SUFFIX)          \
  ATTRIB FROM_TYPE ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(  \
      FROM_TYPE input1, FROM_TYPE input2) {                                    \
    static_assert(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                        \
                  "Incompatible type sizes");                                  \
    union {                                                                    \
      FROM_TYPE from;                                                          \
      TO_TYPE to;                                                              \
    } u1, u2, ures;                                                            \
    u1.from = input1;                                                          \
    u2.from = input2;                                                          \
    TO_TYPE result = RIPPLE_HVX_MATH_EXPAND(                                   \
        INTRINSIC_NAME##_v##FROM_SCALAR_TYPE##SUFFIX)(u1.to, u2.to);           \
    ures.to = result;                                                          \
    return ures.from;                                                          \
  }
#endif

#include "ripple_asin_acos_f16.h"
#include "ripple_asin_acos_f32.h"
#include "ripple_atan_f16.h"
#include "ripple_atan_f32.h"
#include "ripple_div_f32_f16_hvx.h"
#include "ripple_div_f32_f16_hvx_fast.h"
#include "ripple_div_i32_i16.h"
#include "ripple_exp_f32_f16_hvx.h"
#include "ripple_hypot_f32_f16_hvx.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_log_f32_f16_hvx.h"
#include "ripple_pow_f32_f16_hvx.h"
#include "ripple_qhmath_hvx.h"
#include "ripple_sinh_cosh_f32_f16.h"
#include "ripple_sqrt_f32_f16_hvx.h"
#include "ripple_tanh_f32.h"
#include "ripple_tanh_f32_fast.h"
#include "ripple_tanhf16_hvx.h"
#include "ripple_trig_f16.h"
#include "ripple_trunc_round_f32_f16.h"

// ___________________________ HVX Math Library _______________________________

// Required compile flags (previously enforced by the bitcode build, now the
// includer's responsibility in the header-only model):
//   -mhvx -m<archversion>   : HVX must be enabled for the target arch.
//   -fenable-ripple         : required for the ripple_vector_candidate entry
//                             points to be discovered and lowered.
// These implementations use qfloat operations and were built and validated
// with -mhvx-qfloat=legacy.
// The __HEXAGON_ARCH__ < 79 edge-case corrections below rely on
// __HEXAGON_ARCH__ being defined by -m<archversion>.

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

// pass-through scalar versions for the additional vector versions of functions.
// The exact meaning of "fast" and "UNSAFE" (for the vector version) are
// detailed for each function. NOTE: in this header these are scalar versions
// only, mostly with no difference between the 'fast' and regular versions.

float expf(float);

// error on the scale of a factor of 1e-3
// roughly 3x faster than exp_vf
float expf_fast(float x);

// same precision as exp_vf_fast
// roughly 6x faster than exp_vf
// DOES NOT OVERFLOW CORRECTLY: you are responsible for reasonable input!
float expf_fast_UNSAFE(float x);

_Float16 expf16(_Float16);

// errors on the scale of 2% relative to std::exp
// rougly 4x faster than exp_vf16
_Float16 expf16_fast(_Float16 x);

// same precision as exp_vf16_fast
// roughly 6x faster than exp_vf16
// DOES NOT OVERFLOW CORRECTLY: you are responsible for reasonable input!
//  -- fp16 overflows earlier than you think. Undef behavior ~ input 10.3
_Float16 expf16_fast_UNSAFE(_Float16 x);

float logf(float);

// error on the scale of a factor of 7e-3
// roughly 30% faster than log_vf
float logf_fast(float x);

_Float16 logf16(_Float16);

// errors on the scale of 0.7% relative to std::log
// rougly 40% faster than log_vf16
_Float16 logf16_fast(_Float16 x);

float log2f(float);

// error on the scale of a factor of 7e-3
// roughly 30% faster than log2_vf
float log2f_fast(float x);

_Float16 log2f16(_Float16);

// errors on the scale of 0.7% relative to std::log
// rougly 40% faster than log2_vf16
_Float16 log2f16_fast(_Float16 x);

float log10f(float);

// error on the scale of a factor of 7e-3
// roughly 30% faster than log10_vf
float log10f_fast(float x);

_Float16 log10f16(_Float16);

// errors on the scale of 0.7% relative to std::log
// rougly 40% faster than log10_vf16
_Float16 log10f16_fast(_Float16 x);

_Float16 sinhf16(_Float16);

// errors on the scale of 1-2%
// about 30 % faster than default sinh_vf16
_Float16 sinhf16_fast(_Float16 x);

_Float16 coshf16(_Float16);

// errors on the scale of 1-2%
// about 30 % faster than default cosh_vf16
_Float16 coshf16_fast(_Float16 x);

// Errors on the scale of 2e-5
float divf(float x, float y);
float recipf(float x);

// Errors on the scale of 2e-5, same as above
// Roughly 30% faster than div_vf
// Does not handle subnormals (goes to inf) nor inverse subnormals (goes to 0)
// Computes reciprocal first, so accuracy is the same as 1.0 / den
float divf_fast(float x, float y);
float recipf_fast(float x);

// Errors on the scale of 5e-3
// Uses recip_fp32_fast under the hood, with extra optimizations
_Float16 divf16(_Float16 x, _Float16 y);
_Float16 recipf16(_Float16 x);

// Errors on the scale of 1-2%
// More than 2x faster than div_vf16
// Does not handle subnormals (goes to inf) nor inverse subnormals (goes to 0)
// Computes reciprocal first, so accuracy is the same as 1.0 / den
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

_Float16 ldexpf16(_Float16, int);
float ldexpf(float, int);
_Float16 frexpf16(_Float16, int *);
float frexpf(float, int *);

#ifdef __cplusplus
} // extern "C"
#endif

#if __has_bf16__
#if __has_soft_bf16__
#define wrap_spec_unary_bf16_mathfn(op)                                        \
  __attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static __bf16 op##bf16(   \
      const __bf16 Val) {                                                      \
    return (__bf16)op##f((float)Val);                                          \
  }

#define wrap_spec_binary_bf16_mathfn(op)                                       \
  __attribute__((always_inline)) RIPPLE_MAYBE_UNUSED static __bf16 op##bf16(   \
      const __bf16 X, const __bf16 Y) {                                        \
    return (__bf16)op##f((float)(X), (float)(Y));                              \
  }

wrap_spec_binary_bf16_mathfn(hypot);
wrap_spec_unary_bf16_mathfn(rsqrt);

#undef wrap_spec_unary_bf16_mathfn
#undef wrap_spec_binary_bf16_mathfn
#endif
#endif

// _________________ Module-local helper wrappers and vector-candidate
// definitions ____
//
// The helpers (sin_vf, cos_vf, ...) are called by the
// DEFINE_MATH_INTRINSIC_VARIANT_* definitions below.  Both layers are
// module-local so they are inlined into each TU that includes this header and
// are not exported.

/////////////////////////////////////////////////////////////////////////////
// Cast vhf <-> vf
RIPPLE_INTRIN_INLINE HVX_VectorPair vhf_to_vf_pair(HVX_Vector vin) {
  HVX_Vector vqf16 = qhmath_hvx_vqf16_convert_vhf(vin);
  HVX_VectorPair vqf32_p = qhmath_hvx_vqf32_convert_vqf16(vqf16);
  HVX_Vector vqf32_l = Q6_V_lo_W(vqf32_p);
  HVX_Vector vqf32_h = Q6_V_hi_W(vqf32_p);
  HVX_Vector vf_l = qhmath_hvx_vsf_convert_vqf32(vqf32_l);
  HVX_Vector vf_h = qhmath_hvx_vsf_convert_vqf32(vqf32_h);
  HVX_VectorPair vf_p = Q6_W_vcombine_VV(vf_h, vf_l);
  return (vf_p);
}

RIPPLE_INTRIN_INLINE HVX_Vector vf_pair_to_vhf(HVX_VectorPair vin_p) {
  HVX_Vector vf_l = Q6_V_lo_W(vin_p);
  HVX_Vector vf_h = Q6_V_hi_W(vin_p);
  HVX_Vector vqf32_l = qhmath_hvx_vqf32_convert_vsf(vf_l);
  HVX_Vector vqf32_h = qhmath_hvx_vqf32_convert_vsf(vf_h);
  HVX_VectorPair vqf32_p = Q6_W_vcombine_VV(vqf32_h, vqf32_l);
  HVX_Vector vhf = qhmath_hvx_vhf_convert_vqf32(vqf32_p);
  return (vhf);
}

RIPPLE_INTRIN_INLINE HVX_VectorPred ripple_is_zero_vf(HVX_Vector vin) {
  // Matches +0 and -0 by masking the sign bit.
  const HVX_Vector abs_mask = Q6_V_vsplat_R(0x7fffffff);
  HVX_Vector vin_abs = Q6_V_vand_VV(vin, abs_mask);
  return Q6_Q_vcmp_eq_VwVw(vin_abs, Q6_V_vzero());
}

RIPPLE_INTRIN_INLINE HVX_VectorPred ripple_is_zero_vhf(HVX_Vector vin) {
  // Matches +0 and -0 (half precision) by masking the sign bit.
  const HVX_Vector abs_mask = Q6_Vh_vsplat_R(0x7fff);
  HVX_Vector vin_abs = Q6_V_vand_VV(vin, abs_mask);
  return Q6_Q_vcmp_eq_VhVh(vin_abs, Q6_V_vzero());
}

/////////////////////////////////////////////////////////////////////////////
// Sine
RIPPLE_INTRIN_INLINE HVX_Vector sin_vf(HVX_Vector vin) {
  return qhmath_hvx_sin_vf(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sin_vf16(HVX_Vector vin) {
  return ripple_sinf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Cosine
RIPPLE_INTRIN_INLINE HVX_Vector cos_vf(HVX_Vector vin) {
  return qhmath_hvx_cos_vf(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector cos_vf16(HVX_Vector vin) {
  return ripple_cosf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Tangent
RIPPLE_INTRIN_INLINE HVX_Vector tan_vf(HVX_Vector vin) {
  return qhmath_hvx_tan_vf(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector tan_vf16(HVX_Vector vin) {
  HVX_VectorPair vf_p = vhf_to_vf_pair(vin);
  HVX_Vector vf_l = Q6_V_lo_W(vf_p);
  HVX_Vector vf_h = Q6_V_hi_W(vf_p);
  HVX_Vector vout_l = qhmath_hvx_tan_vf(vf_l);
  HVX_Vector vout_h = qhmath_hvx_tan_vf(vf_h);
  HVX_VectorPair vout_p = Q6_W_vcombine_VV(vout_h, vout_l);
  HVX_Vector vout = vf_pair_to_vhf(vout_p);
  return vout;
}

RIPPLE_INTRIN_INLINE HVX_Vector tan_vf16_fast(HVX_Vector vin) {
  return ripple_tanf16_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector cot_vf16_fast(HVX_Vector vin) {
  return ripple_cotf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Tangent
RIPPLE_INTRIN_INLINE HVX_Vector atan_vf(HVX_Vector vin) {
  return ripple_atanf32_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector atan_vf16(HVX_Vector vin) {
  return ripple_atanf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Sine
RIPPLE_INTRIN_INLINE HVX_Vector asin_vf(HVX_Vector vin) {
  return ripple_asinf32_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector asin_vf16(HVX_Vector vin) {
  return ripple_asinf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Cosine
RIPPLE_INTRIN_INLINE HVX_Vector acos_vf(HVX_Vector vin) {
  return ripple_acosf32_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector acos_vf16(HVX_Vector vin) {
  return ripple_acosf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Exponential
RIPPLE_INTRIN_INLINE HVX_Vector exp_vf(HVX_Vector vin) {
  return qhmath_hvx_exp_vf(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf_fast(HVX_Vector vin) {
  return ripple_expf32_hvx_schraudolph<true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf_fast_UNSAFE(HVX_Vector vin) {
  return ripple_expf32_hvx_schraudolph<false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16(HVX_Vector vin) {
  return ripple_expf16_hvx_promote<true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16_fast(HVX_Vector vin) {
  return ripple_expf16_hvx_schraudolph<true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16_fast_UNSAFE(HVX_Vector vin) {
  return ripple_expf16_hvx_schraudolph<false>(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Hyperbolic Cosine
RIPPLE_INTRIN_INLINE HVX_Vector cosh_vf(HVX_Vector vin) {
  HVX_Vector vout = ripple_coshf32_hvx(vin);
#if __HEXAGON_ARCH__ < 79
  // Older qhmath miscomputes cosh(+/-0); force the exact identity == 1.
  const HVX_Vector const1 = Q6_V_vsplat_R(0x3f800000);
  HVX_VectorPred is_zero = ripple_is_zero_vf(vin);
  vout = Q6_V_vmux_QVV(is_zero, const1, vout);
#endif
  return vout;
}

RIPPLE_INTRIN_INLINE HVX_Vector cosh_vf16(HVX_Vector vin) {
  return ripple_coshf16_hvx_promote(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector cosh_vf16_fast(HVX_Vector vin) {
  return ripple_coshf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Hyperbolic Sine
RIPPLE_INTRIN_INLINE HVX_Vector sinh_vf(HVX_Vector vin) {
  return ripple_sinhf32_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sinh_vf16(HVX_Vector vin) {
  return ripple_sinhf16_hvx_promote(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sinh_vf16_fast(HVX_Vector vin) {
  return ripple_sinhf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Hyperbolic Tangent
RIPPLE_INTRIN_INLINE HVX_Vector tanh_vf(HVX_Vector vin) {
  return ripple_tanh_f32_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector tanh_vf_fast(HVX_Vector vin) {
  return ripple_tanh_f32_fast_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector tanh_vf16(HVX_Vector vin) {
  return ripple_tanhf16_hvx<true>(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Sqrt
RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf(HVX_Vector vin) {
  HVX_Vector vout = qhmath_hvx_sqrt_vf(vin);
#if __HEXAGON_ARCH__ < 79
  // Older qhmath miscomputes x={+/-0,1}; override the exact identities.
  const HVX_Vector const1 = Q6_V_vsplat_R(0x3f800000);
  HVX_VectorPred is_zero = ripple_is_zero_vf(vin);
  HVX_VectorPred is_one = Q6_Q_vcmp_eq_VwVw(vin, const1);
  vout = Q6_V_vmux_QVV(is_zero, vin, vout);
  vout = Q6_V_vmux_QVV(is_one, const1, vout);
#endif
  return vout;
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_safe(HVX_Vector vin) {
  return ripple_sqrt_f32<true, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_fast(HVX_Vector vin) {
  return ripple_sqrt_f32<false, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_fast_safe(HVX_Vector vin) {
  return ripple_sqrt_f32<true, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16(HVX_Vector vin) {
  return ripple_sqrt_f16<false, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_safe(HVX_Vector vin) {
  return ripple_sqrt_f16<true, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_fast(HVX_Vector vin) {
  return ripple_sqrt_f16<false, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_fast_safe(HVX_Vector vin) {
  return ripple_sqrt_f16_promote(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Sqrt
RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf(HVX_Vector vin) {
  return ripple_rsqrt_f32<false, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_safe(HVX_Vector vin) {
  return ripple_rsqrt_f32<true, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_fast(HVX_Vector vin) {
  return ripple_rsqrt_f32<false, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_fast_safe(HVX_Vector vin) {
  return ripple_rsqrt_f32<true, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16(HVX_Vector vin) {
  return ripple_rsqrt_f16<false, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_safe(HVX_Vector vin) {
  return ripple_rsqrt_f16<true, true>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_fast(HVX_Vector vin) {
  return ripple_rsqrt_f16<false, false>(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_fast_safe(HVX_Vector vin) {
  return ripple_rsqrt_f16_promote(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Log

// Natural log from qhmath with edge cases corrected: on targets below v79 the
// qhmath implementation miscomputes x={1,+/-0}, so force log(1)=0 and
// log(+/-0)=-inf.
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_log_vf_corrected(HVX_Vector vin) {
  HVX_Vector vlog = qhmath_hvx_log_vf(vin);
#if __HEXAGON_ARCH__ < 79
  const HVX_Vector const1 = Q6_V_vsplat_R(0x3f800000);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector neg_inf = Q6_V_vsplat_R(0xff800000);
  HVX_VectorPred is_one = Q6_Q_vcmp_eq_VwVw(vin, const1);
  HVX_VectorPred is_zero = ripple_is_zero_vf(vin);
  vlog = Q6_V_vmux_QVV(is_one, const0, vlog);
  vlog = Q6_V_vmux_QVV(is_zero, neg_inf, vlog);
#endif
  return vlog;
}

RIPPLE_INTRIN_INLINE HVX_Vector log_vf(HVX_Vector vin) {
  return _ripple_log_vf_corrected(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector log_vf_fast(HVX_Vector vin) {
  return ripple_fast_log_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector _ripple_log_vhf_corrected(HVX_Vector vin) {
  HVX_Vector vlog = qhmath_hvx_log_vhf(vin);
#if __HEXAGON_ARCH__ < 79
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector neg_inf = Q6_Vh_vsplat_R(0xfc00);
  HVX_VectorPred is_one = Q6_Q_vcmp_eq_VhVh(vin, const1);
  HVX_VectorPred is_zero = ripple_is_zero_vhf(vin);
  vlog = Q6_V_vmux_QVV(is_one, const0, vlog);
  vlog = Q6_V_vmux_QVV(is_zero, neg_inf, vlog);
#endif
  return vlog;
}

RIPPLE_INTRIN_INLINE HVX_Vector log_vf16(HVX_Vector vin) {
  return _ripple_log_vhf_corrected(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector log_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Log2
RIPPLE_INTRIN_INLINE HVX_Vector log2_vf(HVX_Vector vin) {
  return _ripple_internal_Vsf_vmpy_VsfVsf(_ripple_log_vf_corrected(vin),
                                          Q6_V_vsplat_R(0x3fb8aa3b));
}

RIPPLE_INTRIN_INLINE HVX_Vector log2_vf_fast(HVX_Vector vin) {
  return ripple_fast_log2_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector log2_vf16(HVX_Vector vin) {
  return _ripple_internal_Vhf_vmpy_VhfVhf(_ripple_log_vhf_corrected(vin),
                                          Q6_Vh_vsplat_R(0x3dc5));
}

RIPPLE_INTRIN_INLINE HVX_Vector log2_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log2_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Log10
RIPPLE_INTRIN_INLINE HVX_Vector log10_vf(HVX_Vector vin) {
  return _ripple_internal_Vsf_vmpy_VsfVsf(_ripple_log_vf_corrected(vin),
                                          Q6_V_vsplat_R(0x3ede5bd9));
}

RIPPLE_INTRIN_INLINE HVX_Vector log10_vf_fast(HVX_Vector vin) {
  return ripple_fast_log10_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector log10_vf16(HVX_Vector vin) {
  return _ripple_internal_Vhf_vmpy_VhfVhf(_ripple_log_vhf_corrected(vin),
                                          Q6_Vh_vsplat_R(0x36f3));
}

RIPPLE_INTRIN_INLINE HVX_Vector log10_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log10_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Pow

// Apply the exact pow identities pow(x,+/-0)==1 and pow(1,y)==1 on top of an
// approximate result. pow is built from log2+exp, whose rounding misses these
// identities on every target, so the correction applies to all widths.
RIPPLE_INTRIN_INLINE HVX_Vector ripple_pow_vf_corrected(HVX_Vector base,
                                                        HVX_Vector exp,
                                                        HVX_Vector vout) {
  const HVX_Vector const1 = Q6_V_vsplat_R(0x3f800000);
  HVX_VectorPred exp_is_zero = ripple_is_zero_vf(exp);
  HVX_VectorPred base_is_one = Q6_Q_vcmp_eq_VwVw(base, const1);
  HVX_VectorPred force_one = Q6_Q_or_QQ(exp_is_zero, base_is_one);
  return Q6_V_vmux_QVV(force_one, const1, vout);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_pow_vhf_corrected(HVX_Vector base,
                                                         HVX_Vector exp,
                                                         HVX_Vector vout) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00); // 1.0 in fp16
  HVX_VectorPred exp_is_zero = ripple_is_zero_vhf(exp);
  HVX_VectorPred base_is_one = Q6_Q_vcmp_eq_VhVh(base, const1);
  HVX_VectorPred force_one = Q6_Q_or_QQ(exp_is_zero, base_is_one);
  return Q6_V_vmux_QVV(force_one, const1, vout);
}

RIPPLE_INTRIN_INLINE HVX_Vector pow_vf(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_pow_vf_corrected(vin1, vin2, ripple_pow_f32<true>(vin1, vin2));
}

RIPPLE_INTRIN_INLINE HVX_Vector pow_vf16(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_pow_vhf_corrected(vin1, vin2,
                                  ripple_pow_f16<true, true>(vin1, vin2));
}

RIPPLE_INTRIN_INLINE HVX_Vector pow_vf16_fast(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_pow_vhf_corrected(vin1, vin2,
                                  ripple_pow_f16<true, false>(vin1, vin2));
}

/////////////////////////////////////////////////////////////////////////////
// Hypot
RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_hypot_f32<false, true>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_safe(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_hypot_f32<true, true>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_fast(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_hypot_f32<false, false>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_fast_safe(HVX_Vector vin1,
                                                   HVX_Vector vin2) {
  return ripple_hypot_f32<true, false>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_hypot_f16<false, true>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_safe(HVX_Vector vin1,
                                                HVX_Vector vin2) {
  return ripple_hypot_f16<true, true>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_fast(HVX_Vector vin1,
                                                HVX_Vector vin2) {
  return ripple_hypot_f16<false, false>(vin1, vin2);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_fast_safe(HVX_Vector vin1,
                                                     HVX_Vector vin2) {
  return ripple_hypot_f16_promote(vin1, vin2);
}

/////////////////////////////////////////////////////////////////////////////
// Truncate
RIPPLE_INTRIN_INLINE HVX_Vector trunc_vf(HVX_Vector vin) {
  return ripple_trunc_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector trunc_vf16(HVX_Vector vin) {
  return ripple_trunc_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Round
RIPPLE_INTRIN_INLINE HVX_Vector round_vf(HVX_Vector vin) {
  return ripple_round_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector round_vf16(HVX_Vector vin) {
  return ripple_round_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Floor
RIPPLE_INTRIN_INLINE HVX_Vector floor_vf(HVX_Vector vin) {
  return ripple_floor_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector floor_vf16(HVX_Vector vin) {
  return ripple_floor_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Ceil
RIPPLE_INTRIN_INLINE HVX_Vector ceil_vf(HVX_Vector vin) {
  return ripple_ceil_f32(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector ceil_vf16(HVX_Vector vin) {
  return ripple_ceil_f16(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Recip / Div
RIPPLE_INTRIN_INLINE HVX_Vector div_vf(HVX_Vector num, HVX_Vector den) {
  return ripple_fp32_div_hvx(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector recip_vf(HVX_Vector vin) {
  return ripple_recip_fp32_div_hvx(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vf_fast(HVX_Vector num, HVX_Vector den) {
  return ripple_fp32_div_hvx_fast(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector recip_vf_fast(HVX_Vector vin) {
  return ripple_recip_fp32_fast(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vf16(HVX_Vector num, HVX_Vector den) {
  return ripple_div_fp16_promote_fast(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector recip_vf16(HVX_Vector vin) {
  return ripple_recip_fp16_promote_fast(vin);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vf16_fast(HVX_Vector num, HVX_Vector den) {
  return ripple_fp16_div_hvx_fast(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector recip_vf16_fast(HVX_Vector vin) {
  return ripple_recip_fp16_fast(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Integer Div
RIPPLE_INTRIN_INLINE HVX_Vector div_vi(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_div_w(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector rem_vi(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_rem_w(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vu(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_div_uw(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector rem_vu(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_rem_uw(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vi16(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_div_h(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector rem_vi16(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_rem_h(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector div_vu16(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_div_uh(num, den);
}

RIPPLE_INTRIN_INLINE HVX_Vector rem_vu16(HVX_Vector num, HVX_Vector den) {
  return ripple_integer_rem_uh(num, den);
}

// _________________ Ripple vector-candidate definitions ______________________

DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, acos, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, acos, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, asin, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, asin, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, atan, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, atan, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cos, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cos, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cosh, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cosh, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cosh, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, cot, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, ceil, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, ceil, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, f, f32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, f, f32t32,
                                     i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, f16, f16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, f16, f16t64,
                                     i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, i, i32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, u, u32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, i16, i16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, div, u16, u16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f, f32t32,
                                    i32t32, ew_pure_, _fast_UNSAFE)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, exp, f16, f16t64,
                                    i32t32, ew_pure_, _fast_UNSAFE)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, floor, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, floor, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f, f32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f, f32t32,
                                     i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f, f32t32,
                                     i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f, f32t32,
                                     i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f16,
                                     f16t64, i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f16,
                                     f16t64, i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f16,
                                     f16t64, i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, hypot, f16,
                                     f16t64, i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log10, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log10, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log10, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log10, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, pow, f, f32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, pow, f16, f16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, pow, f16, f16t64,
                                     i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, recip, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, recip, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, recip, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, recip, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, rem, i, i32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, rem, u, u32t32,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, rem, i16, i16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_BINARY(RIPPLE_CANDIDATE_INLINE, rem, u16, u16t64,
                                     i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, round, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, round, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f, f32t32,
                                    i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f, f32t32,
                                    i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f16, f16t64,
                                    i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, rsqrt, f16, f16t64,
                                    i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sin, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sin, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sinh, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sinh, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sinh, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f, f32t32,
                                    i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f, f32t32,
                                    i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f16, f16t64,
                                    i32t32, ew_pure_, _safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sqrt, f16, f16t64,
                                    i32t32, ew_pure_, _fast_safe)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tan, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tan, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tan, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tanh, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tanh, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, tanh, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, trunc, f, f32t32,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, trunc, f16, f16t64,
                                    i32t32, ew_pure_, )

#undef RIPPLE_HVX_MATH_EXPAND
#undef DEFINE_MATH_INTRINSIC_VARIANT_UNARY
#undef DEFINE_MATH_INTRINSIC_VARIANT_BINARY
