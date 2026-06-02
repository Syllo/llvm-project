#ifndef RIPPLE_TRUNC_ROUND_F32_F16_HVX
#define RIPPLE_TRUNC_ROUND_F32_F16_HVX

//===------------------ ripple_trunc_round_f32_f16_hvx.h --------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
// Internal header to access trunc and round f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include <hexagon_types.h>

RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_sf_from_w(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  return Q6_Vsf_equals_Vw(val);
#else
  const HVX_Vector sign_extract = Q6_V_vsplat_R(0x80000000);
  const HVX_Vector mant_extract = Q6_V_vsplat_R(0x7FFFFF);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const31 = Q6_V_vsplat_R(31);
  const HVX_Vector const32 = Q6_V_vsplat_R(32);
  const HVX_Vector const127 = Q6_V_vsplat_R(127);
  const HVX_Vector const23 = Q6_V_vsplat_R(23);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);

  val = Q6_Vw_vabs_Vw(val);

  HVX_Vector zeros = Q6_Vuw_vcl0_Vuw(val);

  HVX_VectorPred isZero = Q6_Q_vcmp_eq_VwVw(zeros, const32);
  HVX_Vector exp = Q6_Vw_vsub_VwVw(const31, zeros);

  HVX_Vector shift = Q6_Vw_vsub_VwVw(const23, exp);
  val = Q6_Vw_vasl_VwVw(val, shift);
  val = Q6_V_vand_VV(val, mant_extract);

  // we can create all but the mantissa at this stage.
  HVX_Vector shifted_exp =
      Q6_Vw_vasl_VwVw(Q6_Vw_vadd_VwVw(exp, const127), const23);
  HVX_Vector result = Q6_V_vor_VV(sign, shifted_exp);

  // combine everything
  result = Q6_V_vor_VV(result, val);

  // corner case: input is 0
  result = Q6_V_vmux_QVV(isZero, const0, result);

  return result;
#endif
}

RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_hf_from_h(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  return Q6_Vhf_equals_Vh(val);
#else
  const HVX_Vector sign_extract = Q6_Vh_vsplat_R(0x8000);
  const HVX_Vector mant_extract = Q6_Vh_vsplat_R(0x3ff);

  const HVX_Vector const0 = Q6_V_vzero();

  const HVX_Vector const16 = Q6_Vh_vsplat_R(16);
  const HVX_Vector const15 = Q6_Vh_vsplat_R(15);
  const HVX_Vector const10 = Q6_Vh_vsplat_R(10);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);

  val = Q6_Vh_vabs_Vh(val);

  HVX_Vector zeros = Q6_Vuh_vcl0_Vuh(val);

  HVX_VectorPred isZero = Q6_Q_vcmp_eq_VhVh(zeros, const16);
  HVX_Vector exp = Q6_Vh_vsub_VhVh(const15, zeros);

  HVX_Vector shift = Q6_Vh_vsub_VhVh(const10, exp);
  val = Q6_Vh_vasl_VhVh(val, shift);
  val = Q6_V_vand_VV(val, mant_extract);

  // we can create all but the mantissa at this stage.
  HVX_Vector shifted_exp =
      Q6_Vh_vasl_VhVh(Q6_Vh_vadd_VhVh(exp, const15), const10);
  HVX_Vector result = Q6_V_vor_VV(sign, shifted_exp);

  // combine everything
  result = Q6_V_vor_VV(result, val);

  // corner case: input is 0
  result = Q6_V_vmux_QVV(isZero, const0, result);

  return result;
#endif
}

// valid below 2^15
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_h_from_hf(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  return Q6_Vh_equals_Vhf(val);
#else
  const HVX_Vector exp_extract = Q6_Vh_vsplat_R(0x7c00);
  const HVX_Vector mant_extract = Q6_Vh_vsplat_R(0x3ff);
  const HVX_Vector the_one = Q6_Vh_vsplat_R(0x1 << 10);

  const HVX_Vector const10 = Q6_Vh_vsplat_R(10);
  const HVX_Vector const15 = Q6_Vh_vsplat_R(15);
  const HVX_Vector const0 = Q6_V_vzero();

  // we will shift the mantissa mask by the exponent,
  // clamped between 0 and 10;
  // if the exponenet was < 0, we turn val into 0
  HVX_Vector shift = Q6_V_vand_VV(val, exp_extract);

  shift = Q6_Vh_vlsr_VhVh(shift, const10);
  shift = Q6_Vh_vsub_VhVh(shift, const15);

  HVX_VectorPred positive = Q6_Q_vcmp_gt_VhVh(val, const0);

  // fractional values will be right-shifted out
  // large exponents will become a left shift, by the appropriate amount
  shift = Q6_Vh_vsub_VhVh(const10, shift);

  HVX_Vector retval = Q6_V_vor_VV(the_one, Q6_V_vand_VV(val, mant_extract));
  retval = Q6_Vh_vlsr_VhVh(retval, shift);

  HVX_Vector negretval = Q6_Vh_vsub_VhVh(const0, retval);

  return Q6_V_vmux_QVV(positive, retval, negretval);
#endif
}

// valid below 2^31
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_w_from_sf(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  return Q6_Vw_equals_Vsf(val);
#else
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7f800000);
  const HVX_Vector mant_extract = Q6_V_vsplat_R(0x7fffff);
  const HVX_Vector the_one = Q6_V_vsplat_R(0x1 << 23);

  const HVX_Vector const23 = Q6_V_vsplat_R(23);
  const HVX_Vector const127 = Q6_V_vsplat_R(127);
  const HVX_Vector const0 = Q6_V_vzero();

  // we will shift the mantissa mask by the exponent,
  // clamped between 0 and 23;
  // if the exponenet was < 0, we turn val into 0
  HVX_Vector shift = Q6_V_vand_VV(val, exp_extract);

  shift = Q6_Vw_vlsr_VwVw(shift, const23);
  shift = Q6_Vw_vsub_VwVw(shift, const127);

  HVX_VectorPred positive = Q6_Q_vcmp_gt_VwVw(val, const0);

  // fractional values will be right-shifted out
  // large exponents will become a left shift, by the appropriate amount
  shift = Q6_Vw_vsub_VwVw(const23, shift);

  HVX_Vector retval = Q6_V_vor_VV(the_one, Q6_V_vand_VV(val, mant_extract));
  retval = Q6_Vw_vlsr_VwVw(retval, shift);

  HVX_Vector negretval = Q6_Vw_vsub_VwVw(const0, retval);

  return Q6_V_vmux_QVV(positive, retval, negretval);
#endif
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

template <bool unsafe = false>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_trunc_f32(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  if constexpr (unsafe) {
    return Q6_Vsf_equals_Vw(Q6_Vw_equals_Vsf(val));
  } else {
    // this is necessary for correct trunc above 2^31
    HVX_Vector abs_mask = Q6_V_vsplat_R(0x7FFFFFFF);
    // 2^23 - 0.5; largest non-integer expressible in float
    const HVX_Vector sub_2pow23 = Q6_V_vsplat_R(0x4affffff);
    HVX_VectorPred already_w =
        Q6_Q_vcmp_gt_VwVw(Q6_V_vand_VV(val, abs_mask), sub_2pow23);

    HVX_Vector normal_trunc = Q6_Vsf_equals_Vw(Q6_Vw_equals_Vsf(val));
    return Q6_V_vmux_QVV(already_w, val, normal_trunc);
  }
#else
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7F800000);
  HVX_Vector mask = Q6_V_vsplat_R(0x7FFFFF);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const23 = Q6_V_vsplat_R(23);
  const HVX_Vector const127 = Q6_V_vsplat_R(127);

  // we will shift the mantissa mask by the exponent,
  // clamped between 0 and 23;
  // if the exponent < 0, we turn val into 0.
  HVX_Vector shift = Q6_V_vand_VV(val, exp_extract);

  shift = Q6_Vw_vlsr_VwVw(shift, const23);
  shift = Q6_Vw_vsub_VwVw(shift, const127);

  HVX_VectorPred frac = Q6_Q_vcmp_gt_VwVw(const0, shift);
  val = Q6_V_vmux_QVV(frac, const0, val);

  shift = Q6_Vw_vmax_VwVw(shift, const0);
  shift = Q6_Vw_vmin_VwVw(shift, const23);

  mask = Q6_Vw_vlsr_VwVw(mask, shift);
  mask = Q6_V_vnot_V(mask);

  return Q6_V_vand_VV(val, mask);
#endif
}

template <bool unsafe = false>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_trunc_f16(HVX_Vector val) {
#if HEXAGON_ARCH_VERSION >= 73
  if constexpr (unsafe) {
    return Q6_Vhf_equals_Vh(Q6_Vh_equals_Vhf(val));
  } else {
    // this is necessary for correct trunc above 2^15
    HVX_Vector abs_mask = Q6_Vh_vsplat_R(0x7FFF);
    // 1023.5; largest non-integer expressible in hf
    const HVX_Vector sub_2pow10 = Q6_Vh_vsplat_R(0x63ff);
    HVX_VectorPred already_h =
        Q6_Q_vcmp_gt_VhVh(Q6_V_vand_VV(val, abs_mask), sub_2pow10);

    HVX_Vector normal_trunc = Q6_Vhf_equals_Vh(Q6_Vh_equals_Vhf(val));
    return Q6_V_vmux_QVV(already_h, val, normal_trunc);
  }
#else
  const HVX_Vector exp_extract = Q6_Vh_vsplat_R(0x7c00);
  HVX_Vector mask = Q6_Vh_vsplat_R(0x3ff);

  const HVX_Vector const10 = Q6_Vh_vsplat_R(10);
  const HVX_Vector const15 = Q6_Vh_vsplat_R(15);
  const HVX_Vector const0 = Q6_V_vzero();

  // we will shift the mantissa mask by the exponent,
  // clamped between 0 and 10;
  // if the exponenet was < 0, we turn val into 0
  HVX_Vector shift = Q6_V_vand_VV(val, exp_extract);

  shift = Q6_Vh_vlsr_VhVh(shift, const10);
  shift = Q6_Vh_vsub_VhVh(shift, const15);

  HVX_VectorPred frac = Q6_Q_vcmp_gt_VhVh(const0, shift);
  val = Q6_V_vmux_QVV(frac, const0, val);

  shift = Q6_Vh_vmax_VhVh(shift, const0);
  shift = Q6_Vh_vmin_VhVh(shift, const10);

  mask = Q6_Vh_vlsr_VhVh(mask, shift);
  mask = Q6_V_vnot_V(mask);

  return Q6_V_vand_VV(val, mask);
#endif
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_round_f32(HVX_Vector val) {
  const HVX_Vector half_sf = Q6_V_vsplat_R(0x3f000000);

  const HVX_Vector sign_extract = Q6_V_vsplat_R(0x80000000);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make round work correctly, we add 0.5 to positives
  // and -0.5 to negatives
  HVX_Vector correct_half = Q6_V_vor_VV(sign, half_sf);

  val = _ripple_internal_Vsf_vadd_VsfVsf(val, correct_half);
  return ripple_trunc_f32(val);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_round_f16(HVX_Vector val) {
  const HVX_Vector half_hf = Q6_Vh_vsplat_R(0x3800);

  const HVX_Vector sign_extract = Q6_Vh_vsplat_R(0x8000);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make round work correctly, we add 0.5 to positives
  // and -0.5 to negatives
  HVX_Vector correct_half = Q6_V_vor_VV(sign, half_hf);

  val = _ripple_internal_Vhf_vadd_VhfVhf(val, correct_half);
  return ripple_trunc_f16(val);
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_floor_f32(HVX_Vector val) {
  const HVX_Vector neg_half_sf = Q6_V_vsplat_R(0xbf000000);

  const HVX_Vector sign_extract = Q6_V_vsplat_R(0x80000000);

  HVX_Vector trunc = ripple_trunc_f32(val);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make floor work correctly, we add 0 to positives
  // and -1 to negatives
  HVX_Vector adj = Q6_V_vxor_VV(sign, neg_half_sf); // -0.5 for +; +0.5 for -
  adj = _ripple_internal_Vsf_vsub_VsfVsf(neg_half_sf, adj); // 0 for +; -1 for -

  HVX_VectorPred is_integer = Q6_Q_vcmp_eq_VwVw(val, trunc);
  adj = Q6_V_vmux_QVV(is_integer, Q6_V_vzero(), adj); // no adj for ints

  return _ripple_internal_Vsf_vadd_VsfVsf(trunc, adj);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_floor_f16(HVX_Vector val) {
  const HVX_Vector neg_half_hf = Q6_Vh_vsplat_R(0xb800);

  const HVX_Vector sign_extract = Q6_Vh_vsplat_R(0x8000);

  HVX_Vector trunc = ripple_trunc_f16(val);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make floor work correctly, we add 0 to positives
  // and -1 to negatives
  HVX_Vector adj = Q6_V_vxor_VV(sign, neg_half_hf); // -0.5 for +; +0.5 for -
  adj = _ripple_internal_Vhf_vsub_VhfVhf(neg_half_hf, adj); // 0 for +; -1 for -

  HVX_VectorPred is_integer = Q6_Q_vcmp_eq_VhVh(val, trunc);
  adj = Q6_V_vmux_QVV(is_integer, Q6_V_vzero(), adj); // no adj for ints

  return _ripple_internal_Vhf_vadd_VhfVhf(trunc, adj);
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_ceil_f32(HVX_Vector val) {
  const HVX_Vector half_sf = Q6_V_vsplat_R(0x3f000000);

  const HVX_Vector sign_extract = Q6_V_vsplat_R(0x80000000);

  HVX_Vector trunc = ripple_trunc_f32(val);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make floor work correctly, we add 1 to positives
  // and 0 to negatives
  HVX_Vector adj = Q6_V_vor_VV(sign, half_sf);          // 0.5 for +; -0.5 for -
  adj = _ripple_internal_Vsf_vadd_VsfVsf(half_sf, adj); // 1 for +; 0 for -

  HVX_VectorPred is_integer = Q6_Q_vcmp_eq_VwVw(val, trunc);
  adj = Q6_V_vmux_QVV(is_integer, Q6_V_vzero(), adj); // no adj for ints

  return _ripple_internal_Vsf_vadd_VsfVsf(trunc, adj);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_ceil_f16(HVX_Vector val) {
  const HVX_Vector half_hf = Q6_Vh_vsplat_R(0x3800);

  const HVX_Vector sign_extract = Q6_Vh_vsplat_R(0x8000);

  HVX_Vector trunc = ripple_trunc_f16(val);

  HVX_Vector sign = Q6_V_vand_VV(val, sign_extract);
  // trunc rounds positives down and negatives up
  // so, to make ceil work correctly, we add 1 to positives
  // and 0 to negatives
  HVX_Vector adj = Q6_V_vor_VV(sign, half_hf);          // 0.5 for +; -0.5 for -
  adj = _ripple_internal_Vhf_vadd_VhfVhf(half_hf, adj); // 1 for +; 0 for -

  HVX_VectorPred is_integer = Q6_Q_vcmp_eq_VhVh(val, trunc);
  adj = Q6_V_vmux_QVV(is_integer, Q6_V_vzero(), adj); // no adj for ints

  return _ripple_internal_Vhf_vadd_VhfVhf(trunc, adj);
}

#endif
