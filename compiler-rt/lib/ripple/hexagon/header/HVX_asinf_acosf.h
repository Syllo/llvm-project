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
#error "HVX_asinf_acosf.h requires -mhvx-length=128B"
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
float asinf(float);
float acosf(float);
float atanf(float);
float atan2f(float, float);
#ifdef __cplusplus
}
#endif

RIPPLE_INTRIN_INLINE int __ripple_arc_is_nonfinite(float x) {
  unsigned int xbits;
  __builtin_memcpy(&xbits, &x, sizeof(xbits));
  return (xbits & 0x7f800000U) == 0x7f800000U;
}

RIPPLE_INTRIN_INLINE float __ripple_arc_sqrt(float x) {
  unsigned int xbits;

  const float TWO_POWER_64 = 0x1p64f;
  const float TWO_POWER_MINUS_32 = 0x1p-32f;

  // Handle subnormals: scale up, compute, scale back
  int is_small = x < 1e-19f;
  float scale = is_small ? TWO_POWER_MINUS_32 : 1;
  x = is_small ? x * TWO_POWER_64 : x;

  __builtin_memcpy(&xbits, &x, sizeof(xbits));

  // See https://en.wikipedia.org/wiki/Fast_inverse_square_root#Magic_number
  xbits = 0x5F3759DFu - (xbits >> 1);
  float y;
  __builtin_memcpy(&y, &xbits, 4);

  // 3 Newton Raphson iterations for sqrt
  float x2 = x * 0.5f;
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y); // 3rd iteration for < 4 ULP

  return (x * y) * scale;
}

RIPPLE_INTRIN_INLINE float __ripple_arc_rsqrt(float x) {
  unsigned int xbits;

  const float TWO_POWER_64 = 0x1p64f;
  const float TWO_POWER_MINUS_32 = 0x1p-32f;

  // Handle subnormals: scale up, compute, scale back.
  int is_small = x < 1e-19f;
  float scale = is_small ? TWO_POWER_MINUS_32 : 1;
  x = is_small ? x * TWO_POWER_64 : x;

  __builtin_memcpy(&xbits, &x, sizeof(xbits));

  xbits = 0x5F3759DFu - (xbits >> 1);
  float y;
  __builtin_memcpy(&y, &xbits, sizeof(y));

  float x2 = x * 0.5f;
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y);
  y = y * (1.5f - x2 * y * y); // 3rd iteration for < 4 ULP

  return y / scale;
}

// See ripple_ew_pure_acosf on how this is constructed.
static const unsigned short ACOSF_COEFFS_Q_PREPARED[64]
    __attribute__((aligned(128))) = {
        /* Q0 bytes */
        0x00f2,
        0x0004,
        0x00b5,
        0x003f,
        0x0012,
        0x0002,
        0x00b5,
        0x003f,
        0x0028,
        0x00de,
        0x00b4,
        0x003f,
        0x00ac,
        0x00f6,
        0x00b3,
        0x003f,
        /* Q1 bytes */
        0x0067,
        0x0063,
        0x00f1,
        0x003d,
        0x00af,
        0x0052,
        0x00f3,
        0x003d,
        0x00a0,
        0x001d,
        0x0000,
        0x003e,
        0x0024,
        0x0044,
        0x001c,
        0x003e,
        /* Q2 bytes */
        0x00d7,
        0x0011,
        0x00d7,
        0x003c,
        0x00ef,
        0x007a,
        0x00ba,
        0x003c,
        0x00f2,
        0x0043,
        0x002d,
        0x003c,
        0x0098,
        0x00a8,
        0x00ce,
        0x00bc,
        /* Q3 bytes */
        0x00c4,
        0x00f5,
        0x001a,
        0x003c,
        0x00e1,
        0x00f4,
        0x0063,
        0x003c,
        0x00e6,
        0x00f4,
        0x00b2,
        0x003c,
        0x00d7,
        0x0068,
        0x0019,
        0x003d,
};

static const unsigned short ASINF_COEFFS_P_PREPARED[64]
    __attribute__((aligned(128))) = {
        /* P0 bytes */
        0x00ff,
        0x00ff,
        0x007f,
        0x003f,
        0x00ee,
        0x00fb,
        0x007f,
        0x003f,
        0x0023,
        0x00c9,
        0x007f,
        0x003f,
        0x00c1,
        0x0081,
        0x007e,
        0x003f,
        /* P1 bytes */
        0x003f,
        0x00af,
        0x002a,
        0x003e,
        0x0004,
        0x000e,
        0x002c,
        0x003e,
        0x00e0,
        0x002e,
        0x0035,
        0x003e,
        0x00d3,
        0x00fe,
        0x005c,
        0x003e,
        /* P2 bytes */
        0x0040,
        0x002a,
        0x0098,
        0x003d,
        0x0029,
        0x00de,
        0x0083,
        0x003d,
        0x00e8,
        0x0007,
        0x00f5,
        0x003c,
        0x0031,
        0x0023,
        0x0092,
        0x00bd,
        /* P3 bytes */
        0x00e6,
        0x0057,
        0x005a,
        0x003d,
        0x00a2,
        0x002d,
        0x00a1,
        0x003d,
        0x007d,
        0x0015,
        0x00fd,
        0x003d,
        0x00f5,
        0x00f4,
        0x0058,
        0x003e,
};

static const unsigned short ATANF_COEFFS_P_PREPARED[64]
    __attribute__((aligned(128))) = {
        /* P0 bytes */
        0x0000,
        0x0000,
        0x0080,
        0x003f,
        0x00e9,
        0x00ff,
        0x007f,
        0x003f,
        0x0057,
        0x00ff,
        0x007f,
        0x003f,
        0x00b6,
        0x00fd,
        0x007f,
        0x003f,
        /* P1 bytes */
        0x00ca,
        0x0040,
        0x006a,
        0x00bd,
        0x0091,
        0x0030,
        0x006a,
        0x00bd,
        0x008d,
        0x00fa,
        0x0069,
        0x00bd,
        0x00d3,
        0x0092,
        0x0069,
        0x00bd,
        /* P2 bytes */
        0x0008,
        0x00b6,
        0x00c0,
        0x003b,
        0x00d6,
        0x00b9,
        0x00be,
        0x003b,
        0x0049,
        0x005c,
        0x00bb,
        0x003b,
        0x0048,
        0x000a,
        0x00b7,
        0x003b,
        /* P3 bytes */
        0x0053,
        0x0085,
        0x0031,
        0x00ba,
        0x0077,
        0x00df,
        0x001b,
        0x00ba,
        0x0068,
        0x00ca,
        0x0009,
        0x00ba,
        0x00f9,
        0x00b9,
        0x00f4,
        0x00b9,
};

/*
 * Select coefficient 0 through coefficient 3 for 32 lanes. Each word of
 * interval must contain
 * an index in {0, 1, 2, 3}.
 */
RIPPLE_INTRIN_INLINE void
__ripple_arc_lookup_coefs(const uint16_t *prepared_table, v32i32 interval,
                          v32f32 *coeffs0, v32f32 *coeffs1, v32f32 *coeffs2,
                          v32f32 *coeffs3) {
  const HVX_Vector table = *(const HVX_Vector *)(const void *)prepared_table;

  /* Build byte indices {4*j, 4*j+1, 4*j+2, 4*j+3} in each word. */
  const HVX_Vector byte_base = Q6_Vw_vasl_VwR(interval, 2);
  const HVX_Vector base_2bytes =
      Q6_V_vor_VV(byte_base, Q6_Vw_vasl_VwR(byte_base, 8));
  const HVX_Vector base_4bytes =
      Q6_V_vor_VV(base_2bytes, Q6_Vw_vasl_VwR(base_2bytes, 16));
  const HVX_Vector indices0 =
      Q6_Vw_vadd_VwVw(base_4bytes, Q6_V_vsplat_R(0x03020100));

  const HVX_Vector indices1 =
      Q6_Vw_vadd_VwVw(indices0, Q6_V_vsplat_R(0x10101010));
  const HVX_Vector indices2 =
      Q6_Vw_vadd_VwVw(indices0, Q6_V_vsplat_R(0x20202020));
  const HVX_Vector indices3 =
      Q6_Vw_vadd_VwVw(indices0, Q6_V_vsplat_R(0x30303030));

  *coeffs0 = Q6_Vb_vlut32_VbVbI(indices0, table, 0);
  *coeffs1 = Q6_Vb_vlut32_VbVbI(indices1, table, 0);
  *coeffs2 = Q6_Vb_vlut32_VbVbI(indices2, table, 1);
  *coeffs3 = Q6_Vb_vlut32_VbVbI(indices3, table, 1);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_acosf(f32t32 vx) {
  // High-level overview.
  // 1. Use identity acos(-x) = pi - acos(x) to reduce domain from [-1, 1] to
  //    [0, 1].
  // 2. Use X = abs(x)
  // 3. With 4 equal intervals of [0, 1], approximate
  //    acos(X) as sqrt(t) * Q(t), where t = 1 - X and Q is a degree-3
  //    piecewise polynomial.
  // 4. The piecewise code is computed using sollya as `sollya file.sollya`:
  // ----------------- file.sollya --------------------------------------------
  // prec = 200!;
  // display = decimal!;
  // target = 4.8e-7;
  // h = acos(1 - x) / sqrt(x);
  // for j from 0 to 3 do {
  //   lo = j / 4;
  //   hi = (j + 1) / 4;
  //   // h is analytic at zero with h(0) = 1, but its displayed formula
  //   // contains x in the denominator, so omit only the zero endpoint here.
  //   if (j == 0) then i = [1b-30; hi] else i = [lo; hi];
  //   q = fpminimax(h, 3, [|SG...|], i, relative);
  // };
  // --------------------------------------------------------------------------
  // 5. We use the output of sollya to get a v16f32 value and then prepare it
  //    for VLUTs in __ripple_arc_lookup_coefs.

  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0;
  float X = is_neg ? -x : x;
  float result = ripple_broadcast(BS, 0b1, 0.f);

  if (__ripple_arc_is_nonfinite(X) || X > 1) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (X == 1) {
    result = ripple_broadcast(BS, 0b1, 0.f);
  } else {
    // acos(X) = sqrt(t) * Q(t), t = 1 - X.
    float t = 1 - X;
    int t_poly_piece = _VEC_TO_RIPPLE(
        32, int, BS,
        _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(32, float, BS, 4.f * t)));
    t_poly_piece = t_poly_piece > 3 ? 3 : t_poly_piece;

    v32f32 vc0, vc1, vc2, vc3;
    __ripple_arc_lookup_coefs(ACOSF_COEFFS_Q_PREPARED,
                              _RIPPLE_TO_VEC(32, int, BS, t_poly_piece), &vc0,
                              &vc1, &vc2, &vc3);
    float c0 = _VEC_TO_RIPPLE(32, float, BS, vc0);
    float c1 = _VEC_TO_RIPPLE(32, float, BS, vc1);
    float c2 = _VEC_TO_RIPPLE(32, float, BS, vc2);
    float c3 = _VEC_TO_RIPPLE(32, float, BS, vc3);

    result = c2 + c3 * t;
    result = c1 + result * t;
    result = c0 + result * t;
    result *= __ripple_arc_sqrt(t);
  }

  const float pi = 3.141592653589793f;
  result = is_neg ? pi - result : result;
  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_asinf(f32t32 vx) {
  // High-level overview.
  // 1. Use odd symmetry, asin(-x) = -asin(x), to reduce to X = abs(x).
  // 2. Set y = sin(asin(X) / 2) and t = y * y. Then t is in [0, 1/2] and
  //    asin(X) = 2 * y * P(t), where P(t) = asin(sqrt(t)) / sqrt(t).
  // 3. Approximate P with four degree-3 minimax polynomials on intervals of
  //    width 1/8. The coefficients are generated with:
  // ----------------- file.sollya --------------------------------------------
  // prec = 200!;
  // display = decimal!;
  // target = 4.8e-7;
  // h = asin(sqrt(x)) / sqrt(x);
  //
  // for j from 0 to 3 do {
  //   lo = j / 8;
  //   hi = (j + 1) / 8;
  //   // h is analytic at zero with h(0) = 1, but its displayed formula
  //   // contains sqrt(x) in the denominator, so omit only the zero endpoint.
  //   if (j == 0) then i = [1b-30; hi] else i = [lo; hi];
  //   p = fpminimax(h, 3, [|SG...|], i, relative);
  //
  //   print("t interval =", [lo; hi]);
  //   print("P(t) =", p);
  //   print("evaluate asin(x) as 2*y*P(t), t = y^2");
  //   print("where y = x / sqrt(2*(1 + sqrt((1-x)*(1+x))))");
  // };
  // --------------------------------------------------------------------------
  // 4. Restore the input sign.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0.f;
  float X = is_neg ? -x : x;
  float result = ripple_broadcast(BS, 0b1, 0.f);

  if (__ripple_arc_is_nonfinite(X) || X > 1) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (X == 1) {
    result = ripple_broadcast(BS, 0b1, 1.5707963267948966f);
  } else {
    float y = X * __ripple_arc_rsqrt(
                      2.f * (1.f + __ripple_arc_sqrt((1.f - X) * (1.f + X))));
    float t = y * y;
    int t_poly_piece = _VEC_TO_RIPPLE(
        32, int, BS,
        _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(32, float, BS, 8.f * t)));
    t_poly_piece = t_poly_piece > 3 ? 3 : t_poly_piece;

    v32f32 vp0, vp1, vp2, vp3;
    __ripple_arc_lookup_coefs(ASINF_COEFFS_P_PREPARED,
                              _RIPPLE_TO_VEC(32, int, BS, t_poly_piece), &vp0,
                              &vp1, &vp2, &vp3);
    float p0 = _VEC_TO_RIPPLE(32, float, BS, vp0);
    float p1 = _VEC_TO_RIPPLE(32, float, BS, vp1);
    float p2 = _VEC_TO_RIPPLE(32, float, BS, vp2);
    float p3 = _VEC_TO_RIPPLE(32, float, BS, vp3);

    result = p2 + p3 * t;
    result = p1 + result * t;
    result = p0 + result * t;
    result = 2.f * y * result;
  }

  result = is_neg ? -result : result;
  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_INTRIN_INLINE float __ripple_arc_recipf(float x) {
  // Keep the bit-level seed finite for every finite x used by atanf.
  int is_large = x > 0x1p64f;
  float scaled_x = is_large ? x * 0x1p-64f : x;
  float result_scale = is_large ? 0x1p-64f : 1.f;

  int x_bits;
  __builtin_memcpy(&x_bits, &scaled_x, sizeof(x_bits));
  int bits = 0x7EF127EA - x_bits;
  float r;
  __builtin_memcpy(&r, &bits, sizeof(r));

  r = r * (2.0f - scaled_x * r);
  r = r * (2.0f - scaled_x * r);
  r += r * (1.0f - scaled_x * r);
  return r * result_scale;
}

// Approximates atan(r) for nonnegative inputs r <= 1.
RIPPLE_INTRIN_INLINE float __ripple_arctan_minimax(ripple_block_t BS, float r) {
  float y = r * __ripple_arc_recipf(1.f + __ripple_arc_sqrt(1.f + r * r));
  float t = 5.8284271247461903f * y * y;
  int t_poly_piece = _VEC_TO_RIPPLE(
      32, int, BS,
      _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(32, float, BS, 4.f * t)));
  t_poly_piece = t_poly_piece > 3 ? 3 : t_poly_piece;

  v32f32 vp0, vp1, vp2, vp3;
  __ripple_arc_lookup_coefs(ATANF_COEFFS_P_PREPARED,
                            _RIPPLE_TO_VEC(32, int, BS, t_poly_piece), &vp0,
                            &vp1, &vp2, &vp3);
  float p0 = _VEC_TO_RIPPLE(32, float, BS, vp0);
  float p1 = _VEC_TO_RIPPLE(32, float, BS, vp1);
  float p2 = _VEC_TO_RIPPLE(32, float, BS, vp2);
  float p3 = _VEC_TO_RIPPLE(32, float, BS, vp3);

  float result = p2 + p3 * t;
  result = p1 + result * t;
  result = p0 + result * t;
  return 2.f * y * result;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_atanf(f32t32 vx) {
  // High-level overview.
  // 1. Use odd symmetry, atan(-x) = -atan(x), to reduce to X = abs(x).
  // 2. Use reciprocal range reduction, r = X > 1 ? 1 / X : X, so r is in
  //    [0, 1].
  // 3. Set y = r / (1 + sqrt(1 + r * r)) and t = (3 + 2 * sqrt(2)) * y * y.
  //    Then t is in [0, 1] and atan(r) = 2 * y * P(t), where
  //    P(t) = atan((sqrt(2) - 1) * sqrt(t)) /
  //           ((sqrt(2) - 1) * sqrt(t)).
  // 4. Approximate P with four degree-3 minimax polynomials on intervals of
  //    width 1/4. The coefficients are generated with:
  // ----------------- file.sollya --------------------------------------------
  // prec = 200!;
  // display = decimal!;
  // target = 4.8e-7;
  // c = sqrt(2) - 1;
  // h = atan(c * sqrt(x)) / (c * sqrt(x));
  // for j from 0 to 3 do {
  //   lo = j / 4;
  //   hi = (j + 1) / 4;
  //   // h is analytic at zero with h(0) = 1, but its displayed formula
  //   // contains sqrt(x) in the denominator, so omit only the zero endpoint.
  //   if (j == 0) then i = [1b-30; hi] else i = [lo; hi];
  //   p = fpminimax(h, 3, [|SG...|], i, relative);
  // };
  // --------------------------------------------------------------------------
  // 5. Restore the input sign and reciprocal range reduction.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = __builtin_signbit(x);
  float X = is_neg ? -x : x;
  float result = ripple_broadcast(BS, 0b1, 0.f);

  if (__builtin_isnan(X)) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (__ripple_arc_is_nonfinite(X)) {
    result = ripple_broadcast(BS, 0b1, 1.5707963267948966f);
  } else {
    int use_recip = X > 1.f;
    float r = use_recip ? __ripple_arc_recipf(X) : X;
    result = __ripple_arctan_minimax(BS, r);
    result = use_recip ? 1.5707963267948966f - result : result;
  }

  result = is_neg ? -result : result;
  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_atan2f(f32t32 vy, f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float y = _VEC_TO_RIPPLE(32, float, BS, vy);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_x_negative = __builtin_signbit(x);
  int is_y_negative = __builtin_signbit(y);
  float x_abs = is_x_negative ? -x : x;
  float y_abs = is_y_negative ? -y : y;
  const float PI = 3.141592653589793f;
  const float HALF_PI = 1.5707963267948966f;
  const float MIN_NORMAL_F32 = 0x1p-126f;
  int is_x_inf = __builtin_isinf(x);
  int is_y_inf = __builtin_isinf(y);
  float result = ripple_broadcast(BS, 0b1, 0.f);

  // High-level overview:
  // (a) Get is_neg_x, is_neg_y and correspondingly compute abs_x, abs_y.
  // (b) If abs_y == 0, result = 0. If abs_x == 0, result = pi / 2.
  // (c) Compute r = min(abs_x, abs_y) * recip(max(abs_x, abs_y)), scaling
  //     both operands up for a subnormal denominator to avoid overflow.
  // (d) Compute atan(r) using __ripple_arctan_minimax.
  // (e) Use used_recip to undo the range reduction, then shift result to
  //     the correct quadrant by branching on is_neg_x and is_neg_y.
  if (__builtin_isnan(x) || __builtin_isnan(y)) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (y_abs == 0) {
    result = ripple_broadcast(BS, 0b1, 0.f);
  } else if (x_abs == 0) {
    result = ripple_broadcast(BS, 0b1, HALF_PI);
  } else {
    int used_recip = x_abs > y_abs;
    float max_xy = used_recip ? x_abs : y_abs;
    float min_xy = used_recip ? y_abs : x_abs;

    if (max_xy < MIN_NORMAL_F32) {
      // Scale both operands up so that recip(max_xy) does not overflow.
      const float TWO_POWER_126 = 0x1p126f;
      max_xy *= TWO_POWER_126;
      min_xy *= TWO_POWER_126;
    }
    float r = __builtin_isinf(max_xy)
                  ? (is_x_inf && is_y_inf ? 1.f : 0.f)
                  : min_xy * __ripple_arc_recipf(max_xy);

    result = __ripple_arctan_minimax(BS, r);
    result = used_recip ? result : HALF_PI - result;
  }

  result = is_x_negative ? PI - result : result;
  result = is_y_negative ? -result : result;

  return _RIPPLE_TO_VEC(32, float, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
