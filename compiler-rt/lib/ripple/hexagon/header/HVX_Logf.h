//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdint.h>

// Candidate bodies use 128-byte HVX vectors and require -fenable-ripple.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_Logf.h requires -mhvx-length=128B"
#endif

// The vector conversion helpers are templates in C++ and macros in C.
#ifdef __cplusplus
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec<N, T>((BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple<N, T>((BS), (X))
#else
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec(N, T, (BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple(N, T, (BS), (X))
#endif

#ifdef __cplusplus
extern "C" {
#endif
float logf(float);
float log1pf(float);
float log2f(float);
float log10f(float);
float acoshf(float);
float asinhf(float);
float atanhf(float);
_Float16 logf16(_Float16);
_Float16 log2f16(_Float16);
_Float16 log10f16(_Float16);
#ifdef __cplusplus
}
#endif

// returns true only if the bitwise representation of a ieee754 fp32 number is
// NaN or +inf.
RIPPLE_INTRIN_INLINE int __ripple_log_is_posinf_or_nan(unsigned int bits) {
  return bits == 0x7f800000U || (bits & 0x7fffffffU) > 0x7f800000U;
}

RIPPLE_INTRIN_INLINE float _log2f_minimax(float x) {
  // Coefficients obtained using `sollya file.sollya`.
  // ------- file.sollya ------------------
  // prec = 200!;
  // i = [-0.125; 0.125];
  // f = log2(4 * (x + 0.375));
  // n = sup(d);
  // p = fpminimax(f, n, [|SG...|], i, absolute);
  // print("polynomial P(x):", p);
  // ---------------------------------------
  const float c0 = 0.58496248722076416015625f;
  const float c1 = 3.8471839427947998046875f;
  const float c2 = -5.129548549652099609375f;
  const float c3 = 9.12166595458984375f;
  const float c4 = -18.252910614013671875f;
  const float c5 = 38.364444732666015625f;
  const float c6 = -84.27503204345703125f;
  const float c7 = 242.0548095703125f;
  const float c8 = -596.10308837890625f;
  return c0 +
         x * (c1 +
              x * (c2 +
                   x * (c3 +
                        x * (c4 + x * (c5 + x * (c6 + x * (c7 + x * c8)))))));
}

RIPPLE_INTRIN_INLINE float _log2f_from_bits(unsigned int xbits) {
  unsigned int biased_exp = (xbits >> 23) & 0xffU;
  unsigned int mantissa = xbits & 0x7fffffU;
  unsigned int xfrac_bits;
  int e;
  if (biased_exp != 0) {
    e = (int)biased_exp - 127;
    xfrac_bits = 0x3f800000U | mantissa;
  } else {
    // Subnormals.
    int shift = __builtin_clz(mantissa) - 8;
    e = -126 - shift;
    xfrac_bits = 0x3f800000U | ((mantissa << shift) & 0x7fffffU);
  }

  float xfrac;
  __builtin_memcpy(&xfrac, &xfrac_bits, sizeof(xfrac));
  return e + _log2f_minimax(0.25f * xfrac - 0.375f);
}

// Returns scale * log2f(vx)
RIPPLE_INTRIN_INLINE f32t32 __ripple_log_log2f_and_scale(f32t32 vx,
                                                         const float scale) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float log2x = ripple_broadcast(BS, 0b1, 0.f);
  unsigned int xbits;
  __builtin_memcpy(&xbits, &x, sizeof(xbits));

  if (__ripple_log_is_posinf_or_nan(xbits)) {
    log2x = x;
  } else if (x == 0) {
    log2x = -__builtin_inff();
  } else if (x < 0) {
    log2x = -__builtin_nanf("");
  } else if (xbits == 0x3f800000U) {
    // special case for "1"
    log2x = 0.0f;
  } else {
    // High level overview:
    // 1. Write x = xfrac.2^e, where m \in [1, 2]
    // 2. log2(x) = e + _log2f_minimax(0.25 * xfrac - 0.375)
    log2x = _log2f_from_bits(xbits);
  }

  return _RIPPLE_TO_VEC(32, float, BS, log2x *scale);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_logf(f32t32 vx) {
  return __ripple_log_log2f_and_scale(vx, 0.6931471805599453f);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_log1pf(f32t32 vx) {
  // High-level overview.
  // 1. For x near zero, evaluate log1p(x) directly as x * P(x) to avoid the
  //    loss of x when forming 1 + x and cancellation in log(1 + x).
  // 2. Otherwise, compute log(1 + x) with the existing log2 range reduction
  //    and polynomial, scaled by log(2).
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float X = x < 0.0f ? -x : x;
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (x == -1.0f) {
    y = ripple_broadcast(BS, 0b1, -__builtin_inff());
  } else if (x < -1.0f) {
    y = ripple_broadcast(BS, 0b1, -__builtin_nanf(""));
  } else if (X < 0.133f) {
    // Obtain the minimax polynomial by running `sollya file.sollya`.
    // ------------------- file.sollya --------------------------------
    // prec = 200!;
    // i = [-0.133; 0.133];
    // f = log(1 + x) / x;
    // d = guessdegree(1, i, 7.2e-7, 1/f);
    // p = fpminimax(f, sup(d), [|SG...|], i, relative);
    // print("polynomial P(x):", p);
    // ----------------------------------------------------------------
    y = x * (1.0f + x * (-0.4999834001064300537109375f +
                         x * (0.3333143293857574462890625f +
                              x * (-0.2537369430065155029296875f +
                                   x * 0.2035280764102935791015625f))));
  } else {
    f32t32 one_plus_x = _RIPPLE_TO_VEC(32, float, BS, 1.0f + x);
    y = _VEC_TO_RIPPLE(
        32, float, BS,
        __ripple_log_log2f_and_scale(one_plus_x, 0.6931471805599453f));
  }

  return _RIPPLE_TO_VEC(32, float, BS, y);
}

// Approximates sqrt(x) for x in [1, 2^22] within 4 ULP with FP32 arithmetic.
RIPPLE_INTRIN_INLINE float _ripple_log_sqrtin1to2to22(float x) {
  unsigned int xbits;
  __builtin_memcpy(&xbits, &x, sizeof(xbits));

  // See https://en.wikipedia.org/wiki/Fast_inverse_square_root#Magic_number
  xbits = 0x5F3759DFu - (xbits >> 1);
  float y;
  __builtin_memcpy(&y, &xbits, sizeof(y));

  // Three Newton-Raphson iterations refine 1/sqrt(x); two are insufficient.
  float x2 = x * 0.5f;
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y);

  return x * y;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_acoshf(f32t32 vx) {
  // High-level overview:
  // (a) For x >= 2^11, approximate acosh(x) by log(x) + log(2).
  // (b) For x in [2, 2^11), compute log(x + sqrt((x+1)*(x-1))).
  // (c) For x in [1, 2), compute sqrt(t) * P(t), where t = x - 1.
  // (d) Return +0 at x == 1, +inf at +inf, and NaN for x < 1 or NaN.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = ripple_broadcast(BS, 0b1, 0.0f);
  const float LN2 = 0.6931471805599453f;

  if (x == __builtin_inff()) {
    y = x;
  } else if (x >= 0x1p11f) {
    // Add 1 to log2(x) instead of forming 2*x, which could overflow.
    unsigned int xbits;
    __builtin_memcpy(&xbits, &x, sizeof(xbits));
    y = (_log2f_from_bits(xbits) + 1.0f) * LN2;
  } else if (x >= 2.0f) {
    float radicand = (x + 1.0f) * (x - 1.0f);
    float log_arg = x + _ripple_log_sqrtin1to2to22(radicand);
    unsigned int log_arg_bits;
    __builtin_memcpy(&log_arg_bits, &log_arg, sizeof(log_arg_bits));
    y = _log2f_from_bits(log_arg_bits) * LN2;
  } else if (x >= 1.0f) {
    float t = x - 1.0f;

    // Since t >= 2^-23, these exact scalings keep the square-root helper's
    // input in [1, 2^22], including the closest FP32 value above x == 1.
    int is_small = t < 0.25f;
    float scaled_t = is_small ? t * 0x1p24f : t * 4.0f;
    float sqrt_scale = is_small ? 0x1p-12f : 0.5f;
    float sqrt_t = _ripple_log_sqrtin1to2to22(scaled_t) * sqrt_scale;

    // Obtain the minimax polynomial by running `sollya acoshf.sollya`.
    // ------------------- acoshf.sollya ------------------------------
    // prec = 200!;
    // i = [1b-30; 17/10];
    // f = 2 * asinh(sqrt(x / 2)) / sqrt(x);
    // d = guessdegree(1, i, 7.2e-7, 1/f);
    // n = sup(d);
    // p = fpminimax(f, n, [|SG...|], i, relative);
    // print("polynomial P(t):", p);
    // print("evaluate acoshf(x) as sqrtf(t) * P(t), where t = x - 1");
    // print("use this polynomial for 1 <= x < 2.7f");
    // print("polynomial degree:", n);
    // ----------------------------------------------------------------
    float p =
        1.41421282291412353515625f +
        t * (-0.117817394435405731201171875f +
             t * (2.6263274252414703369140625e-2f +
                  t * (-7.170864380896091461181640625e-3f +
                       t * (1.6812793910503387451171875e-3f +
                            t * (-2.1100215963087975978851318359375e-4f)))));
    y = sqrt_t * p;
  } else {
    y = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  }

  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_asinhf(f32t32 vx) {
  // High-level overview:
  // Use odd symmetry: let X = abs(x), then restore x's sign.
  // (a) For X >= 2^{11}, approximate asinh(X) by log(X) + log(2).
  // (b) For X in [0.4f, 2^{11}), compute log(X + sqrt(X*X + 1)).
  // (c) For X in [0.0f, 0.4f), compute X * P(X*X).
  // (d) Preserve signed infinity and signed zero; return NaN for NaN.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0.0f;
  float X = is_neg ? -x : x;
  float y = ripple_broadcast(BS, 0b1, 0.0f);
  const float LN2 = 0.6931471805599453f;
  const float TWO_POWER_11 = 0x1p11f;

  if (X == __builtin_inff()) {
    y = X;
  } else if (X >= TWO_POWER_11) {
    // See (a) above.
    unsigned int xbits;
    __builtin_memcpy(&xbits, &X, sizeof(xbits));
    y = (_log2f_from_bits(xbits) + 1.0f) * LN2;
  } else if (X >= 0.4f) {
    // See (b) above.
    float log_arg = X + _ripple_log_sqrtin1to2to22(X * X + 1.0f);
    unsigned int log_arg_bits;
    __builtin_memcpy(&log_arg_bits, &log_arg, sizeof(log_arg_bits));
    y = _log2f_from_bits(log_arg_bits) * LN2;
  } else {
    // See (c) above.
    float t = X * X;
    // Obtain the minimax polynomial by running `sollya asinhf.sollya`.
    // ------------------- asinhf.sollya ------------------------------
    // prec = 200!;
    // i = [1b-149; 4/25];
    // f = asinh(sqrt(x)) / sqrt(x);
    // d = guessdegree(1, i, 7.2e-7, 1/f);
    // n = sup(d);
    // p = fpminimax(f, n, [|SG...|], i, relative);
    // print("polynomial P(t):", p);
    // print("evaluate asinhf(x) as x * P(t), where t = x * x");
    // print("use this polynomial for 0 <= x < 0.4");
    // ----------------------------------------------------------------
    float p = 0.99999988079071044921875f +
              t * (-0.1666422784328460693359375f +
                   t * (7.4214197695255279541015625e-2f +
                        t * (-3.63321714103221893310546875e-2f)));
    y = X * p;
  }

  y = is_neg ? -y : y;
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_atanhf(f32t32 vx) {
  // High-level overview:
  // Use odd symmetry: let X = abs(x), then restore x's sign.
  // (a) For X > 1.0f, return NaN.
  // (b) For X == 1.0f, return +inf before restoring x's sign.
  // (c) For X in [0.133f, 1.0f), compute 0.5f * log((1 + X)/(1 - X)).
  // (d) For X in [0.0f, 0.133f), compute X * P(X*X) based on the minimax
  // formulation.
  // (e) Preserve signed zero; return NaN for NaN.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0.0f;
  float X = is_neg ? -x : x;
  float y = ripple_broadcast(BS, 0b1, 0.0f);
  const float HALF_LN2 = 0.34657359027997265f;
  const float ATANH_LOG_THRESHOLD = 0.133f;

  if (X > 1.0f) {
    y = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (X == 1.0f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (X >= ATANH_LOG_THRESHOLD) {
    // See (c) above.
    float log_arg = (1.0f + X) / (1.0f - X);
    unsigned int log_arg_bits;
    __builtin_memcpy(&log_arg_bits, &log_arg, sizeof(log_arg_bits));
    y = _log2f_from_bits(log_arg_bits) * HALF_LN2;
  } else {
    // See (d) above.
    float t = X * X;
    // Obtain the minimax polynomial by running `sollya atanhf.sollya`.
    // ------------------- atanhf.sollya ------------------------------
    // prec = 200!;
    // i = [1b-80; (133/1000)^2];
    // f = atanh(sqrt(x)) / sqrt(x);
    // d = guessdegree(1, i, 1.0e-7, 1/f);
    // n = sup(d);
    // p = fpminimax(f, n, [|SG...|], i, relative);
    // print("polynomial P(t):", p);
    // print("evaluate atanhf(x) as x * P(t), where t = x * x");
    // print("use this polynomial for 0 <= x <= 0.133");
    // ----------------------------------------------------------------
    float p = 1.0f + t * (0.3333133757114410400390625f +
                          t * 0.20359279215335845947265625f);
    y = X * p;
  }

  y = is_neg ? -y : y;
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_log2f(f32t32 vx) {
  return __ripple_log_log2f_and_scale(vx, 1.f);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_log10f(f32t32 vx) {
  return __ripple_log_log2f_and_scale(vx, 0.3010299956639812f);
}

// returns true only if the bitwise representation of an ieee754 fp16 number is
// NaN or +inf.
RIPPLE_INTRIN_INLINE int
__ripple_log_is_posinf_or_nan_f16(unsigned short bits) {
  return bits == 0x7c00U || (bits & 0x7fffU) > 0x7c00U;
}

RIPPLE_INTRIN_INLINE _Float16 _log2f16_minimax(_Float16 x) {
  // Coefficients obtained using `sollya file.sollya`.
  // ------- file.sollya ------------------
  // prec = 200!;
  // i = [-0.29296875; 0.4140625];
  // f = log1p(x) / (x * log(2));
  // relative_tolerance = 6 * 2^(-10);
  // d = guessdegree(1, i, relative_tolerance, 1 / f);
  // p = fpminimax(f, sup(d), [|halfprecision...|], i, relative);
  // print("polynomial P(x):", p);
  // ---------------------------------------
  const _Float16 c0 = (_Float16)(1.4443359375f);
  const _Float16 c1 = (_Float16)(-0.7509765625f);
  const _Float16 c2 = (_Float16)(0.447998046875f);
  _Float16 t = x - (_Float16)(1.0f);

  return t * (c0 + t * (c1 + t * c2));
}

RIPPLE_INTRIN_INLINE _Float16 _log2f16_from_bits(unsigned short xbits) {
  unsigned int biased_exp = (xbits >> 10) & 0x1fU;
  unsigned int mantissa = xbits & 0x3ffU;
  unsigned short xfrac_bits;
  int e;
  if (biased_exp != 0) {
    e = (int)biased_exp - 15;
    xfrac_bits = (unsigned short)(0x3c00U | mantissa);
  } else {
    int shift = __builtin_clz(mantissa) - 22;
    e = -15 - shift;
    xfrac_bits =
        (unsigned short)(0x3c00U | ((mantissa << (shift + 1)) & 0x3ffU));
  }

  _Float16 xfrac;
  __builtin_memcpy(&xfrac, &xfrac_bits, sizeof(xfrac));
  if (xfrac > (_Float16)(1.4140625f)) {
    xfrac *= (_Float16)(0.5f);
    ++e;
  }
  return (_Float16)(e) + _log2f16_minimax(xfrac);
}

// Evaluates scale * log2f(x) with fp16 range reduction and minimax evaluation.
RIPPLE_INTRIN_INLINE f16t64
__ripple_log_log2f16_and_scale(f16t64 vx, const _Float16 scale) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 hx = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  _Float16 log2x = ripple_broadcast(BS, 0b1, (_Float16)(0.0f));
  unsigned short xbits;
  __builtin_memcpy(&xbits, &hx, sizeof(xbits));

  if (__ripple_log_is_posinf_or_nan_f16(xbits)) {
    log2x = hx;
  } else if (hx == (_Float16)(0.0f)) {
    log2x = (_Float16)(-__builtin_inff());
  } else if (hx < (_Float16)(0.0f)) {
    log2x = (_Float16)(-__builtin_nanf(""));
  } else if (xbits == 0x3c00U) {
    log2x = (_Float16)(0.0f);
  } else {
    // High-level overview.
    // 1. Write x = xfrac * 2^e, where xfrac is in [1, 2).
    // 2. Shift xfrac to [sqrt(1/2), sqrt(2)] and adjust e.
    // 3. Compute log2(x) as e + _log2f16_minimax(xfrac).
    log2x = _log2f16_from_bits(xbits);
  }

  return _RIPPLE_TO_VEC(64, _Float16, BS, log2x *scale);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_logf16(f16t64 vx) {
  return __ripple_log_log2f16_and_scale(vx, (_Float16)(0.6931471805599453f));
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_log2f16(f16t64 vx) {
  return __ripple_log_log2f16_and_scale(vx, (_Float16)(1.0f));
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_log10f16(f16t64 vx) {
  return __ripple_log_log2f16_and_scale(vx, (_Float16)(0.3010299956639812f));
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
