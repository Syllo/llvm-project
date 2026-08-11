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
#error "HVX_Expf.h requires -mhvx-length=128B"
#endif

#include "ripple_trunc_round_f32_f16.h"

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
float expf(float);
float expm1f(float);
float exp2f(float);
float exp10f(float);
float coshf(float);
float sinhf(float);
#ifdef __cplusplus
}
#endif

RIPPLE_INTRIN_INLINE float __ripple_exp_exp2f_onmodrlehalf(float r) {
  // Coefficients computed using Sollya
  // Run with: `sollya exp2f.sollya` to find a fp32-coefficient polynomial
  // for exp2f(x) on [-.5,.5] to have inf_norm(rtol) < 4.8e-7
  // -------------------- exp2f.sollya ----------------------------
  // prec = 200!;
  // i = [-1/2; 1/2];
  // f = 2^x;
  // d = guessdegree(1, i, 4.8e-7, 1/f);
  // n = sup(d);
  // p = fpminimax(f, n, [|SG...|], i, relative);
  // print("polynomial P(x):", p);
  // ----------------------------------------------------------------
  const float c0 = 1.00000011920928955078125f;
  const float c1 = 0.69314706325531005859375f;
  const float c2 = 0.24022041261196136474609375f;
  const float c3 = 5.550567805767059326171875e-2f;
  const float c4 = 9.678089059889316558837890625e-3f;
  const float c5 = 1.33218802511692047119140625e-3f;
  float y = c0 + r * (c1 + r * (c2 + r * (c3 + r * (c4 + r * c5))));
  return y;
}

// Returns (1/x) for x \in [0.707, 1.414) with error less than 4 ULP.
RIPPLE_INTRIN_INLINE float
__ripple_exp_recipf_on_sqrt_half_to_sqrt_two(float x) {
  int x_bits;
  __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
  int bits = 0x7EF127EA - x_bits;
  float r;
  __builtin_memcpy(&r, &bits, sizeof(r));

  r = r * (2.0f - x * r);
  r = r * (2.0f - x * r);
  r = r * (2.0f - x * r);
  return r;
}

// Returns x * (2^exp) for x \in [0.5, 2) and exp \in [-150, 128], including
// round-to-even subnormal results and overflow to infinity.
RIPPLE_INTRIN_INLINE float __ripple_exp_ldexpf(float x, int exp) {
  unsigned int bits;
  __builtin_memcpy(&bits, &x, sizeof(bits));

  unsigned int mantissa = bits & 0x7fffffU;
  int new_exp = exp + ((int)((bits >> 23) & 1U) - 1);
  unsigned int result_bits;
  if (new_exp > 127) {
    // overflow -> inf.
    result_bits = 0x7f800000U;
  } else if (new_exp < -126) {
    // Try representing result as subnormal; round discarded bits to even.
    int shift = -126 - new_exp;
    if (shift <= 24) {
      unsigned int significand = mantissa | 0x800000U;
      unsigned int halfway = 1U << (shift - 1);
      unsigned int truncated = significand >> shift;
      result_bits = (significand + halfway - 1U + (truncated & 1U)) >> shift;
    } else {
      result_bits = 0U;
    }
  } else {
    result_bits = ((unsigned int)(new_exp + 127) << 23) | mantissa;
  }

  float result;
  __builtin_memcpy(&result, &result_bits, sizeof(result));
  return result;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_expf(f32t32 vx) {
  // High-level overview.
  // 1. Use the identity expf(x) = exp2f(x / log(2))
  // 2. set K = x / log(2)
  // 3. to compute exp2f(K):
  //     a. split K into integer and fractional part as: K = round(K) + Kfrac,
  //        and exp2(K) = exp2f(round(K)) . exp2f(Kfrac)
  //     b. Since, Kfrac \in [-0.5, 0.5], use the minimax polynomial
  //     (__ripple_exp_exp2f_onmodrlehalf)
  //        to obtain exp2f(Kfrac).
  //     c. Compute exp2f(K) by updating the exponent bits of exp2f(Kfrac).
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (x >= 88.73f) {
    // e^x overflows for large +ve x values
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (x <= -104.f) {
    // e^x underflows for large -ve x values.
    y = ripple_broadcast(BS, 0b1, 0.f);
  } else {
    const float ONE_OVER_LOG2 = 1.4426950408889634f;
    const float LN2_HI = 0.693145751953125f;
    const float LN2_LO = 1.428606765330187045e-6f;
    float x_div_ln2 = x * ONE_OVER_LOG2;
    float round_bias = x_div_ln2 < 0.0f ? -0.5f : 0.5f;
    int x_div_ln2_round =
        _VEC_TO_RIPPLE(32, int, BS,
                       _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(
                           32, float, BS, x_div_ln2 + round_bias)));

    // Cody-Waite Style remainder to avoid precision loss in computing the
    // fractional part.
    float x_remainder =
        (x - x_div_ln2_round * LN2_HI) - x_div_ln2_round * LN2_LO;
    float x_div_ln2_frac = x_remainder * ONE_OVER_LOG2;
    float exp2_x_div_ln2_frac = __ripple_exp_exp2f_onmodrlehalf(x_div_ln2_frac);
    y = __ripple_exp_ldexpf(exp2_x_div_ln2_frac, x_div_ln2_round);
  }
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_expm1f(f32t32 vx) {
  // High-level overview.
  // 1. Split x as n * log(2) + r, where n = round(x / log(2)) and
  //    r is in [-log(2)/2, log(2)/2].
  // 2. Evaluate expm1(r) directly with a minimax polynomial. This avoids the
  //    cancellation that would occur when computing exp(r) - 1 near zero.
  // 3. Reconstruct expm1(x) as 2^n * (1 + expm1(r)) - 1. When n is zero,
  //    return expm1(r) directly to preserve accuracy and signed zero.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (x >= 88.73f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (x <= -17.32868f) {
    // exp(x) is at most half an ULP below 1, so expm1(x) rounds to -1.
    y = ripple_broadcast(BS, 0b1, -1.f);
  } else {
    const float ONE_OVER_LOG2 = 1.4426950408889634f;
    const float LN2_HI = 0.693145751953125f;
    const float LN2_LO = 1.428606765330187045e-6f;
    float x_div_ln2 = x * ONE_OVER_LOG2;
    float round_bias = x_div_ln2 < 0.0f ? -0.5f : 0.5f;
    int x_div_ln2_round =
        _VEC_TO_RIPPLE(32, int, BS,
                       _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(
                           32, float, BS, x_div_ln2 + round_bias)));

    // Cody-Waite style remainder to avoid precision loss in the reduction.
    float r = (x - x_div_ln2_round * LN2_HI) - x_div_ln2_round * LN2_LO;

    // Obtain the minimax polynomial by running `sollya file.sollya`.
    // ------------------- file.sollya --------------------------------
    // prec = 200!;
    // i = [-log(2)/2; log(2)/2];
    // f = (exp(x) - 1) / x;
    // p = fpminimax(f, 5, [|SG...|], i, relative);
    // print("polynomial P(x):", p);
    // ----------------------------------------------------------------
    float expm1_r =
        r * (1.0f +
             r * (0.4999999701976776123046875f +
                  r * (0.16666541993618011474609375f +
                       r * (4.166750609874725341796875e-2f +
                            r * (8.366673253476619720458984375e-3f +
                                 r * 1.386416959576308727264404296875e-3f)))));

    y = x_div_ln2_round == 0
            ? expm1_r
            : __ripple_exp_ldexpf(1.0f + expm1_r, x_div_ln2_round) - 1.0f;
  }
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_exp2f(f32t32 vx) {
  // High-level overview.
  // 1. Split x as round(x) + xfrac, so exp2(x) =
  //    exp2(round(x)) * exp2(xfrac).
  // 2. Since xfrac \in [-0.5, 0.5], evaluate exp2(xfrac) with the
  //    minimax polynomial (__ripple_exp_exp2f_onmodrlehalf).
  // 3. Compute exp2(x) by updating the exponent bits of exp2(xfrac).
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (x >= 128.0f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (x <= -150.0f) {
    // 2^-150 is exactly halfway between zero and the least subnormal.
    y = ripple_broadcast(BS, 0b1, 0.f);
  } else {
    float round_bias = x < 0.0f ? -0.5f : 0.5f;
    int x_round = _VEC_TO_RIPPLE(32, int, BS,
                                 _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(
                                     32, float, BS, x + round_bias)));
    float x_frac = x - x_round;
    y = __ripple_exp_ldexpf(__ripple_exp_exp2f_onmodrlehalf(x_frac), x_round);
  }
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_exp10f(f32t32 vx) {
  // High-level overview.
  // 1. Use the identity exp10f(x) = exp2f(x * log2(10)).
  // 2. Split x * log2(10) as round(x * log2(10)) + xfrac.
  // 3. Compute xfrac with a split log10(2) constant to avoid precision
  //    loss, then evaluate exp2(xfrac) with the minimax polynomial.
  // 4. Compute exp10f(x) by updating the exponent bits of exp2(xfrac).
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (x >= 38.53184f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (x <= -45.2f) {
    y = ripple_broadcast(BS, 0b1, 0.f);
  } else {
    const float LOG2_10 = 3.32192809488736234787f;
    const float LOG10_2_HI = 0.301025390625f;
    const float LOG10_2_LO = 4.605038981195e-6f;
    float x_log2_10 = x * LOG2_10;
    float round_bias = x_log2_10 < 0.0f ? -0.5f : 0.5f;
    int x_log2_10_round =
        _VEC_TO_RIPPLE(32, int, BS,
                       _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(
                           32, float, BS, x_log2_10 + round_bias)));
    float x_remainder =
        (x - x_log2_10_round * LOG10_2_HI) - x_log2_10_round * LOG10_2_LO;
    float x_log2_10_frac = x_remainder * LOG2_10;
    y = __ripple_exp_ldexpf(__ripple_exp_exp2f_onmodrlehalf(x_log2_10_frac),
                            x_log2_10_round);
  }
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_coshf(f32t32 vx) {
  // High-level overview.
  // 1. Use even symmetry to reduce to X = abs(x), then set y = X / log(2),
  //    so cosh(X) = (2^y + 2^-y) / 2.
  // 2. Split y as n + f, where n = round(y) and f is in [-0.5, 0.5].
  //    Compute f with a split log(2) constant to avoid precision loss.
  // 3. Evaluate 2^f with the minimax polynomial and obtain 2^-f as its
  //    reciprocal using three Newton-Raphson iterations.
  // 4. Compute cosh(X) as __ripple_exp_ldexpf(2^f, n - 1) +
  //    __ripple_exp_ldexpf(2^-f, -n - 1), folding the division by 2 into the
  //    exponents.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float X = x < 0.f ? -x : x;
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (X >= 89.42f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else {
    const float ONE_OVER_LOG2 = 1.4426950408889634f;
    const float LN2_HI = 0.693145751953125f;
    const float LN2_LO = 1.428606765330187045e-6f;
    float x_div_ln2 = X * ONE_OVER_LOG2;
    int x_div_ln2_round =
        _VEC_TO_RIPPLE(32, int, BS,
                       _ripple_internal_w_from_sf(
                           _RIPPLE_TO_VEC(32, float, BS, x_div_ln2 + 0.5f)));

    // Cody-Waite Style remainder to avoid precision loss in computing the
    // fractional part.
    float x_remainder =
        (X - x_div_ln2_round * LN2_HI) - x_div_ln2_round * LN2_LO;
    float x_div_ln2_frac = x_remainder * ONE_OVER_LOG2;
    float exp2_frac = __ripple_exp_exp2f_onmodrlehalf(x_div_ln2_frac);
    float exp2_neg_frac =
        __ripple_exp_recipf_on_sqrt_half_to_sqrt_two(exp2_frac);
    float exp_x = __ripple_exp_ldexpf(exp2_frac, x_div_ln2_round - 1);
    float exp_neg_x = __ripple_exp_ldexpf(exp2_neg_frac, -x_div_ln2_round - 1);
    y = exp_x + exp_neg_x;
  }
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_sinhf(f32t32 vx) {
  // High-level overview.
  // 1. Use odd symmetry to reduce to X = abs(x), then set y = X / log(2),
  //    so sinh(X) = (2^y - 2^-y) / 2.
  // 2. When X is near zero, the subtraction in (1) will lead to cancellation
  //    and loss of accuracy, use a different polynomial. If X is not near zero
  //    then goto (3).
  // 3. Split y as n + f, where n = round(y) and f is in [-0.5, 0.5].
  //    Compute f with a split log(2) constant to avoid precision loss.
  // 4. Evaluate 2^f with the minimax polynomial and obtain 2^-f as its
  //    reciprocal using three Newton-Raphson iterations.
  // 5. Compute sinh(X) as __ripple_exp_ldexpf(2^f, n - 1) -
  //    __ripple_exp_ldexpf(2^-f, -n - 1), folding the division by 2 into the
  //    exponents.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0.f;
  float X = is_neg ? -x : x;
  float y = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(x)) {
    y = x;
  } else if (X >= 89.42f) {
    y = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (X <= 0.5f) {
    // Obtain the minimax polynomial by running `sollya file.sollya`.
    // ------------------- file.sollya -------------------------------
    // prec = 200!;
    // i = [1b-80; 1/4];
    // f = sinh(sqrt(x)) / sqrt(x);
    // q = fpminimax(f, [|0, 1, 2|], [|SG, SG, SG|], i, relative);
    //
    // print("polynomial Q(t):", q);
    // print("evaluate sinh(x) as x * Q(x * x)");
    // -----------------------------------------------------------------
    float x2 = X * X;
    y = X * (1.00000011920928955078125f +
             x2 * (0.16665934026241302490234375f +
                   x2 * 8.408988825976848602294921875e-3f));
  } else {
    const float ONE_OVER_LOG2 = 1.4426950408889634f;
    const float LN2_HI = 0.693145751953125f;
    const float LN2_LO = 1.428606765330187045e-6f;
    float x_div_ln2 = X * ONE_OVER_LOG2;
    int x_div_ln2_round =
        _VEC_TO_RIPPLE(32, int, BS,
                       _ripple_internal_w_from_sf(
                           _RIPPLE_TO_VEC(32, float, BS, x_div_ln2 + 0.5f)));

    // Cody-Waite Style remainder to avoid precision loss in computing the
    // fractional part.
    float x_remainder =
        (X - x_div_ln2_round * LN2_HI) - x_div_ln2_round * LN2_LO;
    float x_div_ln2_frac = x_remainder * ONE_OVER_LOG2;
    float exp2_frac = __ripple_exp_exp2f_onmodrlehalf(x_div_ln2_frac);
    float exp2_neg_frac =
        __ripple_exp_recipf_on_sqrt_half_to_sqrt_two(exp2_frac);
    float exp_x = __ripple_exp_ldexpf(exp2_frac, x_div_ln2_round - 1);
    float exp_neg_x = __ripple_exp_ldexpf(exp2_neg_frac, -x_div_ln2_round - 1);
    y = exp_x - exp_neg_x;
  }
  y = is_neg ? -y : y;
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
