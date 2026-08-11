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
#include "HVX_Cbrtf.h"
#include "HVX_Expf.h"
#include "HVX_Logf.h"
#include "HVX_MathTrig.h"
#include "HVX_MathTrigfp16.h"
#include "HVX_NearbyIntf.h"
#include "HVX_Sqrtf.h"
#include "HVX_asinf_acosf.h"
#include "HVX_copysign.h"
#include "HVX_fabs.h"
#include "HVX_fdim.h"
#include "HVX_fmaxfmin.h"
#include "HVX_ldexp_frexp.h"
#include "HVX_logbf.h"
#include "HVX_nextafter.h"
#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <ripple.h>
#include <stdbool.h>

#define RIPPLE_HVX_MATH_EXPAND(NAME) NAME

#ifdef __cplusplus
#define DEFINE_MATH_INTRINSIC_VARIANT_UNARY(ATTRIB, INTRINSIC_NAME,            \
                                            FROM_SCALAR_TYPE, FROM_TYPE,       \
                                            TO_TYPE, LIBTAG, SUFFIX)           \
  ATTRIB FROM_TYPE ripple_##LIBTAG##INTRINSIC_NAME##FROM_SCALAR_TYPE##SUFFIX(  \
      FROM_TYPE input) {                                                       \
    RIPPLE_STATIC_ASSERT(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                 \
                         "Incompatible type sizes");                           \
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
    RIPPLE_STATIC_ASSERT(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                 \
                         "Incompatible type sizes");                           \
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
    RIPPLE_STATIC_ASSERT(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                 \
                         "Incompatible type sizes");                           \
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
    RIPPLE_STATIC_ASSERT(sizeof(FROM_TYPE) == sizeof(TO_TYPE),                 \
                         "Incompatible type sizes");                           \
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
#include "ripple_atan_f16.h"
#include "ripple_div_f32_f16_hvx.h"
#include "ripple_div_f32_f16_hvx_fast.h"
#include "ripple_div_i32_i16.h"
#include "ripple_exp_f32_f16_hvx.h"
#include "ripple_hypot_f32_f16_hvx.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_log_f32_f16_hvx.h"
#include "ripple_pow_f32_f16_hvx.h"
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

float sqrtf(float);
float cbrtf(float);
float sqrtf_safe(float);
float sqrtf_fast(float);
float sqrtf_fast_safe(float);

_Float16 sqrtf16(_Float16);
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

float tanhf(float);
float tanhf_fast(float);
_Float16 tanhf16(_Float16);

// Trigonometric functions.
float sinf(float);
_Float16 sinf16(_Float16);
float cosf(float);
_Float16 cosf16(_Float16);
float tanf(float);
_Float16 tanf16(_Float16);

// Inverse trigonometric functions.
float asinf(float);
_Float16 asinf16(_Float16);
float acosf(float);
_Float16 acosf16(_Float16);
float atanf(float);
float atan2f(float, float);
_Float16 atanf16(_Float16);

// Hyperbolic functions.
float sinhf(float);
float coshf(float);

// Rounding functions.
float ceilf(float);
_Float16 ceilf16(_Float16);
float floorf(float);
_Float16 floorf16(_Float16);
float roundf(float);
_Float16 roundf16(_Float16);
float truncf(float);
_Float16 truncf16(_Float16);

// Power function.
float powf(float, float);
_Float16 powf16(_Float16, _Float16);

float fmaf(float, float, float);
_Float16 fmaf16(_Float16, _Float16, _Float16);

// pass-through scalar versions for the additional vector versions of functions.
// The exact meaning of "fast" and "UNSAFE" (for the vector version) are
// detailed for each function. NOTE: in this header these are scalar versions
// only, mostly with no difference between the 'fast' and regular versions.

float expf(float);
float expm1f(float);
float exp2f(float);
float exp10f(float);

// error on the scale of a factor of 1e-3
// roughly 3x faster than the precise f32 exp implementation
float expf_fast(float x);

// same precision as exp_vf_fast
// roughly 6x faster than the precise f32 exp implementation
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
float log1pf(float);

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

int divi(int x, int y);
unsigned int divu(unsigned int x, unsigned int y);
int remi(int x, int y);
unsigned int remu(unsigned int x, unsigned int y);

short divi16(short x, short y);
unsigned short divu16(unsigned short x, unsigned short y);
short remi16(short x, short y);
unsigned short remu16(unsigned short x, unsigned short y);

_Float16 powf16_fast(_Float16, _Float16);

_Float16 tanf16_fast(_Float16);
_Float16 cotf16_fast(_Float16);

_Float16 ldexpf16(_Float16, int);
float ldexpf(float, int);
_Float16 scalbnf16(_Float16, int);
float scalbnf(float, int);
_Float16 frexpf16(_Float16, int *);
float frexpf(float, int *);

#ifdef __cplusplus
} // extern "C"
#endif

#if __has_bf16__
#if __has_soft_bf16__
#define wrap_spec_unary_bf16_mathfn(op)                                        \
  RIPPLE_INTRIN_INLINE __bf16 op##bf16(const __bf16 Val) {                     \
    return (__bf16)op##f((float)Val);                                          \
  }

#define wrap_spec_binary_bf16_mathfn(op)                                       \
  RIPPLE_INTRIN_INLINE __bf16 op##bf16(const __bf16 X, const __bf16 Y) {       \
    return (__bf16)op##f((float)(X), (float)(Y));                              \
  }

wrap_spec_binary_bf16_mathfn(hypot);
wrap_spec_unary_bf16_mathfn(rsqrt);

#undef wrap_spec_unary_bf16_mathfn
#undef wrap_spec_binary_bf16_mathfn
#endif
#endif

// __________________ double / long double math functions ___________________
//
// Hexagon has long double == double, and this header otherwise only declares
// float/_Float16 variants. Define always-inline wrappers around the
// corresponding __builtin_ripple_* / __builtin_ripple_*l builtins so that
// unsuffixed (sin, sqrt, ...) and 'l'-suffixed (sinl, sqrtl, ...) call sites
// still surface a Ripple builtin call, matching what ripple_math.h's macros
// used to provide.
#define emulate_spec_unary_double_mathfn(op)                                   \
  __attribute__((always_inline)) static inline double op(double Val) {         \
    return __builtin_ripple_##op(Val);                                         \
  }

#define emulate_spec_binary_double_mathfn(op)                                  \
  __attribute__((always_inline)) static inline double op(double X, double Y) { \
    return __builtin_ripple_##op(X, Y);                                        \
  }

#define emulate_spec_unary_longdouble_mathfn(op)                               \
  __attribute__((always_inline)) static inline long double op##l(              \
      long double Val) {                                                       \
    return __builtin_ripple_##op##l(Val);                                      \
  }

#define emulate_spec_binary_longdouble_mathfn(op)                              \
  __attribute__((always_inline)) static inline long double op##l(              \
      long double X, long double Y) {                                          \
    return __builtin_ripple_##op##l(X, Y);                                     \
  }

// Only do this when <math.h> isn't available: when it is, it already
// declares/defines these names (some libcs, e.g. hexagon's, give real bodies
// to a subset of them), and our wrappers would collide with it
// ("redefinition of 'cos'", "static declaration of 'sqrt' follows
// non-static declaration", etc).
#if !__has_include(<math.h>)
emulate_spec_unary_double_mathfn(sqrt);
emulate_spec_unary_longdouble_mathfn(sqrt);
emulate_spec_unary_double_mathfn(asin);
emulate_spec_unary_longdouble_mathfn(asin);
emulate_spec_unary_double_mathfn(acos);
emulate_spec_unary_longdouble_mathfn(acos);
emulate_spec_unary_double_mathfn(atan);
emulate_spec_unary_longdouble_mathfn(atan);
emulate_spec_binary_double_mathfn(atan2);
emulate_spec_binary_longdouble_mathfn(atan2);
emulate_spec_unary_double_mathfn(sin);
emulate_spec_unary_longdouble_mathfn(sin);
emulate_spec_unary_double_mathfn(cos);
emulate_spec_unary_longdouble_mathfn(cos);
emulate_spec_unary_double_mathfn(tan);
emulate_spec_unary_longdouble_mathfn(tan);
emulate_spec_unary_double_mathfn(sinh);
emulate_spec_unary_longdouble_mathfn(sinh);
emulate_spec_unary_double_mathfn(cosh);
emulate_spec_unary_longdouble_mathfn(cosh);
emulate_spec_unary_double_mathfn(tanh);
emulate_spec_unary_longdouble_mathfn(tanh);
emulate_spec_binary_double_mathfn(pow);
emulate_spec_binary_longdouble_mathfn(pow);
emulate_spec_unary_double_mathfn(log);
emulate_spec_unary_longdouble_mathfn(log);
emulate_spec_unary_double_mathfn(log10);
emulate_spec_unary_longdouble_mathfn(log10);
emulate_spec_unary_double_mathfn(log2);
emulate_spec_unary_longdouble_mathfn(log2);
emulate_spec_unary_double_mathfn(exp);
emulate_spec_unary_longdouble_mathfn(exp);
emulate_spec_unary_double_mathfn(exp2);
emulate_spec_unary_longdouble_mathfn(exp2);
emulate_spec_unary_double_mathfn(exp10);
emulate_spec_unary_longdouble_mathfn(exp10);
emulate_spec_unary_double_mathfn(fabs);
emulate_spec_unary_longdouble_mathfn(fabs);
emulate_spec_binary_double_mathfn(copysign);
emulate_spec_binary_longdouble_mathfn(copysign);
emulate_spec_unary_double_mathfn(floor);
emulate_spec_unary_longdouble_mathfn(floor);
emulate_spec_unary_double_mathfn(ceil);
emulate_spec_unary_longdouble_mathfn(ceil);
emulate_spec_unary_double_mathfn(trunc);
emulate_spec_unary_longdouble_mathfn(trunc);
emulate_spec_unary_double_mathfn(rint);
emulate_spec_unary_longdouble_mathfn(rint);
emulate_spec_unary_double_mathfn(nearbyint);
emulate_spec_unary_longdouble_mathfn(nearbyint);
emulate_spec_unary_double_mathfn(round);
emulate_spec_unary_longdouble_mathfn(round);
emulate_spec_unary_double_mathfn(roundeven);
emulate_spec_unary_longdouble_mathfn(roundeven);

__attribute__((always_inline)) static inline double ldexp(double X, int Y) {
  return __builtin_ripple_ldexp(X, Y);
}
__attribute__((always_inline)) static inline long double ldexpl(long double X,
                                                                int Y) {
  return __builtin_ripple_ldexpl(X, Y);
}

__attribute__((always_inline)) static inline double frexp(double X, int *Y) {
  return __builtin_ripple_frexp(X, Y);
}
__attribute__((always_inline)) static inline long double frexpl(long double X,
                                                                int *Y) {
  return __builtin_ripple_frexpl(X, Y);
}

#undef emulate_spec_unary_double_mathfn
#undef emulate_spec_binary_double_mathfn
#undef emulate_spec_unary_longdouble_mathfn
#undef emulate_spec_binary_longdouble_mathfn
#endif // !__has_include(<math.h>)

// _______________ float / _Float16 math functions (yet to be implemented) ____
//
// TODO: The HVX math library does not yet provide real implementations for
// these float / _Float16 math functions. For now, emulate them with
// always-inline wrappers around the corresponding __builtin_ripple_*f /
// __builtin_ripple_*f16 builtins, mirroring the double/long double wrappers
// above. Replace with proper HVX math library entry points when available.
#define emulate_spec_unary_float_mathfn(op)                                    \
  __attribute__((always_inline)) static inline float op##f(float Val) {        \
    return __builtin_ripple_##op##f(Val);                                      \
  }

#define emulate_spec_binary_float_mathfn(op)                                   \
  __attribute__((always_inline)) static inline float op##f(float X, float Y) { \
    return __builtin_ripple_##op##f(X, Y);                                     \
  }

#define emulate_spec_unary_f16_mathfn(op)                                      \
  __attribute__((always_inline)) static inline _Float16 op##f16(               \
      _Float16 Val) {                                                          \
    return __builtin_ripple_##op##f16(Val);                                    \
  }

#define emulate_spec_binary_f16_mathfn(op)                                     \
  __attribute__((always_inline)) static inline _Float16 op##f16(_Float16 X,    \
                                                                _Float16 Y) {  \
    return __builtin_ripple_##op##f16(X, Y);                                   \
  }

emulate_spec_binary_f16_mathfn(atan2);
emulate_spec_unary_f16_mathfn(exp2);
emulate_spec_unary_f16_mathfn(exp10);
emulate_spec_unary_f16_mathfn(roundeven);

// Only do this when <math.h> isn't available: when it is, it already
// declares/defines these names (some libcs, e.g. hexagon's, give real bodies
// to a subset of them), and our wrappers would collide with it
// ("redefinition of 'cos'", "static declaration of 'sqrt' follows
// non-static declaration", etc).
#if !__has_include(<math.h>)
emulate_spec_unary_float_mathfn(roundeven);
#endif // math.h

#undef emulate_spec_unary_float_mathfn
#undef emulate_spec_binary_float_mathfn
#undef emulate_spec_unary_f16_mathfn
#undef emulate_spec_binary_f16_mathfn

// _________________ Module-local helper wrappers and vector-candidate
// definitions ____
//
// The helpers (sqrt_vf_safe, tanh_vf, ...) are called by the
// DEFINE_MATH_INTRINSIC_VARIANT_* definitions below.  Both layers are
// module-local so they are inlined into each TU that includes this header and
// are not exported.

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
// Tangent
RIPPLE_INTRIN_INLINE HVX_Vector tan_vf16_fast(HVX_Vector vin) {
  return ripple_tanf16_hvx(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector cot_vf16_fast(HVX_Vector vin) {
  return ripple_cotf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Tangent
RIPPLE_INTRIN_INLINE HVX_Vector atan_vf16(HVX_Vector vin) {
  return ripple_atanf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Sine
RIPPLE_INTRIN_INLINE HVX_Vector asin_vf16(HVX_Vector vin) {
  return ripple_asinf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Cosine
RIPPLE_INTRIN_INLINE HVX_Vector acos_vf16(HVX_Vector vin) {
  return ripple_acosf16_hvx(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Exponential
RIPPLE_INTRIN_INLINE HVX_Vector exp_vf_fast(HVX_Vector vin) {
  return ripple_expf32_hvx_schraudolph(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf_fast_UNSAFE(HVX_Vector vin) {
  return ripple_expf32_hvx_schraudolph(vin, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16(HVX_Vector vin) {
  return ripple_expf16_hvx_promote(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16_fast(HVX_Vector vin) {
  return ripple_expf16_hvx_schraudolph(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector exp_vf16_fast_UNSAFE(HVX_Vector vin) {
  return ripple_expf16_hvx_schraudolph(vin, false);
}

/////////////////////////////////////////////////////////////////////////////
// Hyperbolic Cosine
RIPPLE_INTRIN_INLINE HVX_Vector cosh_vf16(HVX_Vector vin) {
  return ripple_coshf16_hvx_promote(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector cosh_vf16_fast(HVX_Vector vin) {
  return ripple_coshf16_hvx(vin, true);
}

/////////////////////////////////////////////////////////////////////////////
// Hyperbolic Sine
RIPPLE_INTRIN_INLINE HVX_Vector sinh_vf16(HVX_Vector vin) {
  return ripple_sinhf16_hvx_promote(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector sinh_vf16_fast(HVX_Vector vin) {
  return ripple_sinhf16_hvx(vin, true);
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
  return ripple_tanhf16_hvx(vin, true);
}

/////////////////////////////////////////////////////////////////////////////
// Sqrt
RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_safe(HVX_Vector vin) {
  return ripple_sqrt_f32(vin, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_fast(HVX_Vector vin) {
  return ripple_sqrt_f32(vin, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf_fast_safe(HVX_Vector vin) {
  return ripple_sqrt_f32(vin, true, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16(HVX_Vector vin) {
  return ripple_sqrt_f16(vin, false, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_safe(HVX_Vector vin) {
  return ripple_sqrt_f16(vin, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_fast(HVX_Vector vin) {
  return ripple_sqrt_f16(vin, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector sqrt_vf16_fast_safe(HVX_Vector vin) {
  return ripple_sqrt_f16_promote(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Inverse Sqrt
RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf(HVX_Vector vin) {
  return ripple_rsqrt_f32(vin, false, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_safe(HVX_Vector vin) {
  return ripple_rsqrt_f32(vin, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_fast(HVX_Vector vin) {
  return ripple_rsqrt_f32(vin, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf_fast_safe(HVX_Vector vin) {
  return ripple_rsqrt_f32(vin, true, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16(HVX_Vector vin) {
  return ripple_rsqrt_f16(vin, false, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_safe(HVX_Vector vin) {
  return ripple_rsqrt_f16(vin, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_fast(HVX_Vector vin) {
  return ripple_rsqrt_f16(vin, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector rsqrt_vf16_fast_safe(HVX_Vector vin) {
  return ripple_rsqrt_f16_promote(vin);
}

/////////////////////////////////////////////////////////////////////////////
// Log

RIPPLE_INTRIN_INLINE HVX_Vector log_vf_fast(HVX_Vector vin) {
  return ripple_fast_log_f32(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector log_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log_f16(vin, true);
}

/////////////////////////////////////////////////////////////////////////////
// Log2
RIPPLE_INTRIN_INLINE HVX_Vector log2_vf_fast(HVX_Vector vin) {
  return ripple_fast_log2_f32(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector log2_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log2_f16(vin, true);
}

/////////////////////////////////////////////////////////////////////////////
// Log10
RIPPLE_INTRIN_INLINE HVX_Vector log10_vf_fast(HVX_Vector vin) {
  return ripple_fast_log10_f32(vin, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector log10_vf16_fast(HVX_Vector vin) {
  return ripple_fast_log10_f16(vin, true);
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
  return ripple_pow_vf_corrected(vin1, vin2, ripple_pow_f32(vin1, vin2, true));
}

RIPPLE_INTRIN_INLINE HVX_Vector pow_vf16(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_pow_vhf_corrected(vin1, vin2,
                                  ripple_pow_f16(vin1, vin2, true, true));
}

RIPPLE_INTRIN_INLINE HVX_Vector pow_vf16_fast(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_pow_vhf_corrected(vin1, vin2,
                                  ripple_pow_f16(vin1, vin2, true, false));
}

/////////////////////////////////////////////////////////////////////////////
// Hypot
RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_hypot_f32(vin1, vin2, false, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_safe(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_hypot_f32(vin1, vin2, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_fast(HVX_Vector vin1,
                                              HVX_Vector vin2) {
  return ripple_hypot_f32(vin1, vin2, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf_fast_safe(HVX_Vector vin1,
                                                   HVX_Vector vin2) {
  return ripple_hypot_f32(vin1, vin2, true, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16(HVX_Vector vin1, HVX_Vector vin2) {
  return ripple_hypot_f16(vin1, vin2, false, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_safe(HVX_Vector vin1,
                                                HVX_Vector vin2) {
  return ripple_hypot_f16(vin1, vin2, true, true);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_fast(HVX_Vector vin1,
                                                HVX_Vector vin2) {
  return ripple_hypot_f16(vin1, vin2, false, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector hypot_vf16_fast_safe(HVX_Vector vin1,
                                                     HVX_Vector vin2) {
  return ripple_hypot_f16_promote(vin1, vin2);
}

/////////////////////////////////////////////////////////////////////////////
// Truncate
RIPPLE_INTRIN_INLINE HVX_Vector trunc_vf(HVX_Vector vin) {
  return ripple_trunc_f32(vin, false);
}

RIPPLE_INTRIN_INLINE HVX_Vector trunc_vf16(HVX_Vector vin) {
  return ripple_trunc_f16(vin, false);
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
  return ripple_recip_fp32_fast(vin, true);
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

// The vector conversion helpers are templates in C++ and macros in C.
#ifdef __cplusplus
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec<N, T>((BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple<N, T>((BS), (X))
#else
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec(N, T, (BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple(N, T, (BS), (X))
#endif

// Use multiply followed by add; these candidates do not guarantee the
// single rounding of a fused operation.
RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_fmaf(f32t32 va, f32t32 vb,
                                               f32t32 vc) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float a = _VEC_TO_RIPPLE(32, float, BS, va);
  float b = _VEC_TO_RIPPLE(32, float, BS, vb);
  float c = _VEC_TO_RIPPLE(32, float, BS, vc);
  float result = a * b + c;
  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_fmaf16(f16t64 va, f16t64 vb,
                                                 f16t64 vc) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 a = _VEC_TO_RIPPLE(64, _Float16, BS, va);
  _Float16 b = _VEC_TO_RIPPLE(64, _Float16, BS, vb);
  _Float16 c = _VEC_TO_RIPPLE(64, _Float16, BS, vc);
  _Float16 result = a * b + c;
  return _RIPPLE_TO_VEC(64, _Float16, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE

DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, acos, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, asin, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, atan, f16, f16t64,
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
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f, f32t32,
                                    i32t32, ew_pure_, _fast)
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, log2, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
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
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sinh, f16, f16t64,
                                    i32t32, ew_pure_, )
DEFINE_MATH_INTRINSIC_VARIANT_UNARY(RIPPLE_CANDIDATE_INLINE, sinh, f16, f16t64,
                                    i32t32, ew_pure_, _fast)
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
