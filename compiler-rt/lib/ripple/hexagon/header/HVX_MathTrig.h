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
#include <stddef.h>
#include <stdint.h>

// Candidate bodies use 128-byte HVX vectors and require -fenable-ripple.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_MathTrig.h requires -mhvx-length=128B"
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
#define _RIPPLE_TO_VEC_2D(N, T, BS, X) ripple_to_vec_2d<N, T>((BS), (X))
#define _VEC_TO_RIPPLE_2D(N, T, BS, X) vec_to_ripple_2d<N, T>((BS), (X))
#else
#define _RIPPLE_TO_VEC_2D(N, T, BS, X) ripple_to_vec_2d(N, T, (BS), (X))
#define _VEC_TO_RIPPLE_2D(N, T, BS, X) vec_to_ripple_2d(N, T, (BS), (X))
#endif

#ifdef __cplusplus
extern "C" {
#endif
float sinf(float);
float cosf(float);
float tanf(float);
#ifdef __cplusplus
}
#endif

enum {
  TWO_OVER_PI_00 = 0xa2f9,
  TWO_OVER_PI_01 = 0x836e,
  TWO_OVER_PI_02 = 0x4e44,
  TWO_OVER_PI_03 = 0x1529,
  TWO_OVER_PI_04 = 0xfc27,
  TWO_OVER_PI_05 = 0x57d1,
  TWO_OVER_PI_06 = 0xf534,
  TWO_OVER_PI_07 = 0xddc0,
  TWO_OVER_PI_08 = 0xdb62,
  TWO_OVER_PI_09 = 0x9599,
  TWO_OVER_PI_10 = 0x3c43,
  TWO_OVER_PI_11 = 0x9041,
  TWO_OVER_PI_12 = 0xfe51,
  TWO_OVER_PI_13 = 0x63ab,
};
static const float PI_OVER_2_F32 = 1.57079637050628662109375f;
static const float PI_OVER_4_F32 = 0.785398185253143310546875f;

// Pre-shuffled constant table for vlut16 (Rt=0), one entry per 32-bit word
// in the low halfword. 64 hwords = 128 bytes = one full HVX vector.
static const unsigned short TWO_OVER_PI_LUT_PREPARED[64]
    __attribute__((aligned(128))) = {
        TWO_OVER_PI_00,
        0,
        TWO_OVER_PI_01,
        0,
        TWO_OVER_PI_02,
        0,
        TWO_OVER_PI_03,
        0,
        TWO_OVER_PI_04,
        0,
        TWO_OVER_PI_05,
        0,
        TWO_OVER_PI_06,
        0,
        TWO_OVER_PI_07,
        0,
        TWO_OVER_PI_08,
        0,
        TWO_OVER_PI_09,
        0,
        TWO_OVER_PI_10,
        0,
        TWO_OVER_PI_11,
        0,
        TWO_OVER_PI_12,
        0,
        TWO_OVER_PI_13,
        0,
        // remaining 36 hwords are don't-care for idx in [0,13]
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
};

// Deconstructs x as: x = (-1)^s * m * 2^e. Normal values have a
// 24-bit significand including the implicit leading 1; subnormals do not.
RIPPLE_INTRIN_INLINE void __ripple_trig_deconstruct_float(float x, short *s,
                                                          int16_t *e,
                                                          unsigned int *m) {
  unsigned int bits;
  __builtin_memcpy(&bits, &x, sizeof(bits));

  *s = (bits >> 31) & 1;
  short expo = (bits >> 23) & 0xFF;

  if (expo == 0) {
    // Subnormals and zero have no implicit leading 1.
    *m = bits & 0x7FFFFF;
    *e = -149;
  } else {
    // Normal values include the implicit leading 1. For NaN and infinity the
    // decomposition is only used to retain expo == 0xFF as e == 105.
    *m = (bits & 0x7FFFFF) | 0x800000;
    *e = expo - 127 - 23;
  }
}

RIPPLE_INTRIN_INLINE size_t __ripple_trig_shuffoe(size_t i, size_t n) {
  return i % 2 ? (i / 2 + n / 2) : i / 2;
}

RIPPLE_INTRIN_INLINE float __ripple_trig_recipf(float x) {
  // Three iterations are required to reach float precision from this guess.

  // bit hacking guess.
  int x_bits;
  __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
  int bits = 0x7EF127EA - x_bits;
  float r;
  __builtin_memcpy(&r, &bits, sizeof(r));

  // Newton Raphson, iteration #1.
  r = r * (2.0f - x * r);
  // Newton Raphson, iteration #2.
  r = r * (2.0f - x * r);
  // Newton Raphson, iteration #3.
  r = r * (2.0f - x * r);
  return r;
}

// 64-entry 16-bit lookup.  Uses vlut16. Copied from HVX_Vlut.cc as it was
// leading to some include errors.
RIPPLE_INTRIN_INLINE v128u16 __ripple_trig_vlut_64u16(v128u8 indices,
                                                      v64u16 prepared) {
  v128u16 result = Q6_Wh_vlut16_VbVhR(indices, prepared, 0);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 1);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 2);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 3);
  return Q6_W_vshuff_VVR(Q6_V_hi_W(result), Q6_V_lo_W(result), -2);
}

RIPPLE_INTRIN_INLINE f32t32 __ripple_trig_paynehanekf(f32t32 vx,
                                                      i32t32 *vquadrant) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  // Additional blocks for vlut.
  ripple_block_t BSlookup_tbl = ripple_set_block_shape(0, 64);
  ripple_block_t BSlookup_idxstack = ripple_set_block_shape(0, 32, 4);
  ripple_block_t BSlookup_idxs = ripple_set_block_shape(0, 64, 2);
  ripple_block_t BSlookup_out = ripple_set_block_shape(0, 32, 2);

  const unsigned short *TWO_OVER_PI_LUT_PREPARED_PTR =
      ripple_ptr_alignment(&TWO_OVER_PI_LUT_PREPARED[0], 128);
  unsigned short two_over_pi_tbl_prepared =
      TWO_OVER_PI_LUT_PREPARED_PTR[ripple_id(BSlookup_tbl, 0)];

  short sign, e;
  unsigned int m;

  const float x = _VEC_TO_RIPPLE(32, float, BS, vx);

  __ripple_trig_deconstruct_float(x, &sign, &e, &m);

  // e == 105 corresponds to an all-ones FP32 exponent (255 - 127 - 23).
  // It therefore covers both NaN and infinity; finite FP32 values have e <=
  // 104.
  const int is_nonfinite = e == 105;

  // if is_small == true, mulitplying by 2/pi and then dividing loses sig bits
  // -> dont attempt.
  const int is_small = (-PI_OVER_4_F32 <= x) & (x <= PI_OVER_4_F32);
  // Keep the two low quotient bits needed for the quadrant. Limbs preceding
  // this window are guaranteed to contribute a multiple of four.
  const unsigned char window_base = (unsigned char)(e > 2 ? (e - 2) / 16 : 0);
  unsigned short windows[6];
#pragma clang loop unroll(full)
  for (unsigned char i = 0; i < 6; i += 2) {
    unsigned char indices =
        ripple_stack(BSlookup_idxstack, window_base + i, window_base + i + 1u,
                     window_base + i + 1u, window_base + i + 2u);
    unsigned short looked_up = _VEC_TO_RIPPLE_2D(
        128, unsigned short, BSlookup_idxs,
        __ripple_trig_vlut_64u16(
            _RIPPLE_TO_VEC_2D(128, unsigned char, BSlookup_idxstack, indices),
            _RIPPLE_TO_VEC(64, unsigned short, BSlookup_tbl,
                           two_over_pi_tbl_prepared)));
    unsigned int window_i_ip1_pair = ripple_reinterp_u32(
        BSlookup_out, ripple_shuffle(looked_up, __ripple_trig_shuffoe));
    windows[i] = (unsigned short)ripple_slice(window_i_ip1_pair, -1, 0);
    windows[i + 1] = (unsigned short)ripple_slice(window_i_ip1_pair, -1, 1);
  }

  unsigned short product[8];
#pragma clang loop unroll(full)
  for (int i = 0; i < 8; ++i)
    product[i] = ripple_broadcast(BS, 0b1, (unsigned short)0);

  const unsigned short m_lo = (unsigned short)m;
  const unsigned short m_hi = (unsigned short)(m >> 16);
#pragma clang loop unroll(full)
  for (int h = 0; h < 2; ++h) {
    const unsigned short mantissa_limb = h == 0 ? m_lo : m_hi;
    unsigned int carry = ripple_broadcast(BS, 0b1, (unsigned int)0);
#pragma clang loop unroll(full)
    for (int i = 0; i < 6; ++i) {
      unsigned int t = mantissa_limb * windows[5 - i];
      t += product[h + i];
      t += carry;
      product[h + i] = (unsigned short)t;
      carry = t >> 16;
    }
    unsigned int t = carry + product[h + 6];
    product[h + 6] = (unsigned short)t;
    if (h == 0)
      product[7] += (unsigned short)(t >> 16);
  }

  // window represents 2/pi scaled by 2^(96 + 16*window_base).
  const int nfrac_bits = 96 + 16 * window_base - e;
  const unsigned short n = (unsigned short)(nfrac_bits & 15);
  unsigned short fl_q = ripple_broadcast(BS, 0b1, UINT16_C(0));
  unsigned short fl4 = ripple_broadcast(BS, 0b1, UINT16_C(0));
  unsigned short fl3 = ripple_broadcast(BS, 0b1, UINT16_C(0));
  unsigned short fl2 = ripple_broadcast(BS, 0b1, UINT16_C(0));
  unsigned short fl1 = ripple_broadcast(BS, 0b1, UINT16_C(0));
  unsigned short fl0 = ripple_broadcast(BS, 0b1, UINT16_C(0));
#pragma clang loop unroll(full)
  for (int ilimb = 0; ilimb < 8; ++ilimb) {
    int is_pt = (16 * ilimb <= nfrac_bits) & (nfrac_bits < 16 * (ilimb + 1));
    if (is_pt) {
      fl_q = ilimb + 1 < 8 ? product[ilimb + 1] : (unsigned short)0;
      fl4 = product[ilimb];
      fl3 = ilimb >= 1 ? product[ilimb - 1] : (unsigned short)0;
      fl2 = ilimb >= 2 ? product[ilimb - 2] : (unsigned short)0;
      fl1 = ilimb >= 3 ? product[ilimb - 3] : (unsigned short)0;
      fl0 = ilimb >= 4 ? product[ilimb - 4] : (unsigned short)0;
    }
  }

  // Extract floating point bits
  unsigned short hihi, hilo, lohi, lolo; /* MSB-first fraction limbs */
  hihi = (n == 0) ? fl3 : ((fl4 << (16 - n)) | (fl3 >> n));
  hilo = (n == 0) ? fl2 : ((fl3 << (16 - n)) | (fl2 >> n));
  lohi = (n == 0) ? fl1 : ((fl2 << (16 - n)) | (fl1 >> n));
  lolo = (n == 0) ? fl0 : ((fl1 << (16 - n)) | (fl0 >> n));

  // If f > 0.5 (the argument reduction rounding, we need to use (1-f))
  const unsigned short frac_greater_than_half = (unsigned short)(hihi >> 15);
  if (frac_greater_than_half) {
    // Note that in this case, fractional part would be 1-f.
    // But simply doing 1-f will lead to catastrophic cancellation.
    //  Instead take the 64 bits we have of the mantissa, M,  and compute
    //  2^64-M.
    // Compute 2^64-M using ~M +1 (thanks to bitwise not.)

    // manipulate the limbs to get (2^64-M-1)
    lolo = ~lolo;
    lohi = ~lohi;
    hilo = ~hilo;
    hihi = ~hihi;

    // add resulting limbs with 1. take care of the carry (if needed.)
    unsigned short carry, new_carry;
    carry = (lolo == 0xffff);
    lolo++;

    new_carry = (lohi == 0xffff) & carry;
    lohi = lohi + carry;
    carry = new_carry;

    new_carry = (hilo == 0xffff) & carry;
    hilo = hilo + carry;
    carry = new_carry;
    hihi = hihi + carry;
  }

  // Extract the quadrant info
  unsigned short q_lo = (unsigned short)((fl4 >> n) & 0x3u);
  unsigned short q_hi = (unsigned short)((n == 15) ? ((fl_q & 1u) << 1) : 0u);
  unsigned short quadrant =
      ((q_lo | q_hi) + frac_greater_than_half) & UINT16_C(0x3u);

  // reconstruct the fractional part from 4 limbs of the mantissa.
  float f = (float)lolo;
  f = f * 0x1p-16f + (float)lohi;
  f = f * 0x1p-16f + (float)hilo;
  f = f * 0x1p-16f + (float)hihi;
  f = f * 0x1p-16f;

  float r = f * PI_OVER_2_F32;
  if (frac_greater_than_half)
    r = -r;

  if (sign) {
    r = -r;
    quadrant = (4u - quadrant) & 0x3u;
  }
  float y = is_nonfinite ? __builtin_nanf("") : (is_small ? x : r);

  *vquadrant =
      _RIPPLE_TO_VEC(32, int, BS, (is_nonfinite || is_small) ? 0 : quadrant);
  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_INTRIN_INLINE float __ripple_trig_sinf_on_absx_le_pi_by_4(float x) {
  const float c3 = -0.16666667163372039794921875f;
  const float c5 = 8.333347737789154052734375e-3f;
  const float c7 = -1.9842604524455964565277099609375e-4f;
  const float c9 = 2.760012648650445044040679931640625e-6f;
  const float c11 = -2.50293279435709337121807038784027099609375e-8f;

  float xsq = x * x;
  float y = xsq * c11 + c9;
  y = xsq * y + c7;
  y = xsq * y + c5;
  y = xsq * y + c3;
  y = xsq * y + 1;
  y *= x;

  return y;
}

RIPPLE_INTRIN_INLINE float __ripple_trig_cosf_on_absx_le_pi_by_4(float x) {
  const float c2 = -0.5f;
  const float c4 = 4.166664183139801025390625e-2f;
  const float c6 = -1.388833043165504932403564453125e-3f;
  const float c8 = 2.47562347794882953166961669921875e-5f;
  const float c10 = -2.59630184018533327616751194000244140625e-7f;

  float xsq = x * x;
  float y = xsq * c10 + c8;
  y = xsq * y + c6;
  y = xsq * y + c4;
  y = xsq * y + c2;
  y = xsq * y + 1;

  return y;
}

RIPPLE_INTRIN_INLINE float __ripple_trig_tanf_on_absx_le_pi_by_4(float x) {
  const float c3 = 0.33333075046539306640625f;
  const float c5 = 0.13339905440807342529296875f;
  const float c7 = 5.3348250687122344970703125e-2f;
  const float c9 = 2.46033705770969390869140625e-2f;
  const float c11 = 2.892402000725269317626953125e-3f;
  const float c13 = 9.500005282461643218994140625e-3f;

  float xsq = x * x;
  float y = xsq * c13 + c11;
  y = xsq * y + c9;
  y = xsq * y + c7;
  y = xsq * y + c5;
  y = xsq * y + c3;
  y = xsq * y + 1;
  y *= x;

  return y;
}

RIPPLE_INTRIN_INLINE float __ripple_trig_cotf_on_absx_le_pi_by_4(float x) {
  const float c1 = -0.3333333432674407958984375f;
  const float c3 = -2.222204394638538360595703125e-2f;
  const float c5 = -2.11752182804048061370849609375e-3f;
  const float c7 = -2.0846328698098659515380859375e-4f;
  const float c9 = -2.548247357481159269809722900390625e-5f;
  const float c11 = -3.5257363606433500535786151885986328125e-7f;

  float xsq = x * x;

  float y = xsq * c11 + c9;
  y = xsq * y + c7;
  y = xsq * y + c5;
  y = xsq * y + c3;
  y = xsq * y + c1;
  y = x * y + __ripple_trig_recipf(x);

  return y;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_sinf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  i32t32 vquadrant;
  float r = _VEC_TO_RIPPLE(
      32, float, BS,
      __ripple_trig_paynehanekf(_RIPPLE_TO_VEC(32, float, BS, x), &vquadrant));
  int quadrant = _VEC_TO_RIPPLE(32, int, BS, vquadrant);

  // Select the reduced polynomial and sign from the quadrant bits.
  // Two selects also avoid a four-way branch in header candidates.
  float sine = __ripple_trig_sinf_on_absx_le_pi_by_4(r);
  float cosine = __ripple_trig_cosf_on_absx_le_pi_by_4(r);
  float y = (quadrant & 1) ? cosine : sine;
  y = (quadrant & 2) ? -y : y;

  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_cosf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  i32t32 vquadrant;
  float r = _VEC_TO_RIPPLE(
      32, float, BS,
      __ripple_trig_paynehanekf(_RIPPLE_TO_VEC(32, float, BS, x), &vquadrant));
  int quadrant = _VEC_TO_RIPPLE(32, int, BS, vquadrant);

  // Select the reduced polynomial and sign from the quadrant bits.
  // Two selects also avoid a four-way branch in header candidates.
  float sine = __ripple_trig_sinf_on_absx_le_pi_by_4(r);
  float cosine = __ripple_trig_cosf_on_absx_le_pi_by_4(r);
  float y = (quadrant & 1) ? sine : cosine;
  y = ((quadrant + 1) & 2) ? -y : y;

  return _RIPPLE_TO_VEC(32, float, BS, y);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_tanf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  i32t32 vquadrant;
  float r = _VEC_TO_RIPPLE(
      32, float, BS,
      __ripple_trig_paynehanekf(_RIPPLE_TO_VEC(32, float, BS, x), &vquadrant));
  int quadrant = _VEC_TO_RIPPLE(32, int, BS, vquadrant);

  float y = ripple_broadcast(BS, 0b1, 0.f);
  switch (quadrant & 0x1) {
  // quadrant gives the offset and then apply the following.
  // 1. sin(a + b) = sin(a)cos(b)+cos(a)sin(b).
  // 2. cos(a+b) = cos(a)cos(b)-sin(a)sin(b).
  case 0:
    // y = sin(x + 0) / cos(x+0) = tan(x)
    y = __ripple_trig_tanf_on_absx_le_pi_by_4(r);
    break;
  default:
    // y = sin(x + pi/2) / cos(x+pi/2) = -cot(x)
    y = -__ripple_trig_cotf_on_absx_le_pi_by_4(r);
    break;
  }

  return _RIPPLE_TO_VEC(32, float, BS, y);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
#undef _RIPPLE_TO_VEC_2D
#undef _VEC_TO_RIPPLE_2D
