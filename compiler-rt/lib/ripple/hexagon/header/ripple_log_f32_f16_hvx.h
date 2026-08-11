#ifndef RIPPLE_LOG_F32_F16_HVX
#define RIPPLE_LOG_F32_F16_HVX

//===---------------------- ripple_log_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access a log f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_trunc_round_f32_f16.h"
#include <hexagon_types.h>
#include <stdbool.h>

// assumes qf32 input val
// produces qf32 output val
// end result around 0.7% accurate
RIPPLE_INTRIN_INLINE HVX_Vector ripple_log2_qf32_approximation(HVX_Vector val) {
  const HVX_Vector const_qf32_1 =
      Q6_Vqf32_vadd_VsfVsf(Q6_V_vsplat_R(0x3f800000), Q6_V_vzero());
  const HVX_Vector const_qf32_4xadj =
      Q6_Vqf32_vadd_VsfVsf(Q6_V_vsplat_R(0x3eb045fe), Q6_V_vzero());
  // 4 * 0.086071; largest error

  val = Q6_Vqf32_vsub_Vqf32Vqf32(val, const_qf32_1);

  HVX_Vector inv = Q6_Vqf32_vsub_Vqf32Vqf32(const_qf32_1, val);
  HVX_Vector temp = Q6_Vqf32_vmpy_Vqf32Vqf32(val, inv);
  temp = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, const_qf32_4xadj);

  return Q6_Vqf32_vadd_Vqf32Vqf32(val, temp);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log2_f32(HVX_Vector val,
                                                     bool safe) {
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector mant_extract = Q6_V_vsplat_R(0x7FFFFF);
  const HVX_Vector mant_adjust = Q6_V_vsplat_R(127 << 23);

  const HVX_Vector const23 = Q6_V_vsplat_R(23);
  const HVX_Vector const127 = Q6_V_vsplat_R(127);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1 = Q6_V_vsplat_R(1);
  const HVX_Vector const2pow23 = Q6_V_vsplat_R(0x4b000000);
  const HVX_Vector constNegInf = Q6_V_vsplat_R(0xff800000);

  HVX_VectorPred is_subnormal;
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VwVw(
      const1, val); // if val == 0 or sign bit set, val looks like negative word
  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VwVw(const0, Q6_V_vand_VV(val, exp_extract));
    HVX_Vector alt_val = _ripple_internal_Vsf_vmpy_VsfVsf(val, const2pow23);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }

  // extract exp-based log
  HVX_Vector log_2_exp = Q6_Vw_vsub_VwVw(
      Q6_Vw_vasr_VwR(Q6_V_vand_VV(val, exp_extract), 23), const127);
  if (safe) {
    HVX_Vector alt_log_2_exp = Q6_Vw_vsub_VwVw(log_2_exp, const23);
    log_2_exp = Q6_V_vmux_QVV(is_subnormal, alt_log_2_exp, log_2_exp);
  }
  log_2_exp = _ripple_internal_sf_from_w(log_2_exp);

  // take the mantissa and move to standard location
  val = Q6_V_vor_VV(Q6_V_vand_VV(val, mant_extract), mant_adjust);

  // begin QF math
  val = Q6_Vqf32_vadd_VsfVsf(val, const0);
  val = ripple_log2_qf32_approximation(val);
  val = Q6_Vsf_equals_Vqf32(val);
  // end QF math

  val = _ripple_internal_Vsf_vadd_VsfVsf(val, log_2_exp);
  val = Q6_V_vmux_QVV(is_illegal, constNegInf, val);
  return val;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log_f32(HVX_Vector val, bool safe) {
  return _ripple_internal_Vsf_vmpy_VsfVsf(ripple_fast_log2_f32(val, safe),
                                          Q6_V_vsplat_R(0x3f317218));
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log10_f32(HVX_Vector val,
                                                      bool safe) {
  return _ripple_internal_Vsf_vmpy_VsfVsf(ripple_fast_log2_f32(val, safe),
                                          Q6_V_vsplat_R(0x3e9a209b));
}

//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_log_f16_promote(HVX_Vector x) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  HVX_VectorPair qfx = Q6_Wqf32_vmpy_VhfVhf(x, const1);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_fast_log_f32(Q6_Vsf_equals_Vqf32(Q6_V_hi_W(qfx)), false),
      Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_fast_log_f32(Q6_Vsf_equals_Vqf32(Q6_V_lo_W(qfx)), false),
      Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vdeal_VVR(hiRes, loRes, 0);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

// polynomial approximation between 1.0 and 2.0
// end result about 0.7% accurate
RIPPLE_INTRIN_INLINE HVX_Vector ripple_log2_qf16_approximation(HVX_Vector val) {
  const HVX_Vector const_qf16_1 =
      Q6_Vqf16_vadd_VhfVhf(Q6_Vh_vsplat_R(0x3c00), Q6_V_vzero());
  const HVX_Vector const_qf16_4xadj =
      Q6_Vqf16_vadd_VhfVhf(Q6_Vh_vsplat_R(0x3582), Q6_V_vzero());
  // 4 * 0.086071; largest error

  val = Q6_Vqf16_vsub_Vqf16Vqf16(val, const_qf16_1);

  HVX_Vector inv = Q6_Vqf16_vsub_Vqf16Vqf16(const_qf16_1, val);
  HVX_Vector temp = Q6_Vqf16_vmpy_Vqf16Vqf16(val, inv);
  temp = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, const_qf16_4xadj);

  return Q6_Vqf16_vadd_Vqf16Vqf16(val, temp);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log2_f16(HVX_Vector val,
                                                     bool safe) {
  const HVX_Vector exp_extract = Q6_Vh_vsplat_R(0x7c00);
  const HVX_Vector mant_extract = Q6_Vh_vsplat_R(0x3ff);
  const HVX_Vector mant_adjust = Q6_Vh_vsplat_R(15 << 10);

  const HVX_Vector const10 = Q6_Vh_vsplat_R(10);
  const HVX_Vector const15 = Q6_Vh_vsplat_R(15);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1 = Q6_Vh_vsplat_R(1);
  const HVX_Vector const2pow10 = Q6_Vh_vsplat_R(0x6400);
  const HVX_Vector constNegInf = Q6_Vh_vsplat_R(0xfc00);

  HVX_VectorPred is_subnormal;
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VhVh(
      const1, val); // if val == 0 or sign bit set, val looks like negative word
  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VhVh(const0, Q6_V_vand_VV(val, exp_extract));
    HVX_Vector alt_val = _ripple_internal_Vhf_vmpy_VhfVhf(val, const2pow10);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }

  // extract exp-based log
  HVX_Vector log_2_exp = Q6_Vh_vsub_VhVh(
      Q6_Vh_vasr_VhR(Q6_V_vand_VV(val, exp_extract), 10), const15);
  if (safe) {
    HVX_Vector alt_log_2_exp = Q6_Vh_vsub_VhVh(log_2_exp, const10);
    log_2_exp = Q6_V_vmux_QVV(is_subnormal, alt_log_2_exp, log_2_exp);
  }

  log_2_exp = _ripple_internal_hf_from_h(log_2_exp);

  // take the mantissa and move to standard location
  val = Q6_V_vor_VV(Q6_V_vand_VV(val, mant_extract), mant_adjust);

  // begin QF math
  val = Q6_Vqf16_vadd_VhfVhf(val, const0);
  val = ripple_log2_qf16_approximation(val);
  val = Q6_Vhf_equals_Vqf16(val);
  // end QF math

  val = _ripple_internal_Vhf_vadd_VhfVhf(val, log_2_exp);

  val = Q6_V_vmux_QVV(is_illegal, constNegInf, val);
  return val;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log_f16(HVX_Vector val, bool safe) {
  return _ripple_internal_Vhf_vmpy_VhfVhf(ripple_fast_log2_f16(val, safe),
                                          Q6_Vh_vsplat_R(0x398c));
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fast_log10_f16(HVX_Vector val,
                                                      bool safe) {
  return _ripple_internal_Vhf_vmpy_VhfVhf(ripple_fast_log2_f16(val, safe),
                                          Q6_Vh_vsplat_R(0x34d1));
}

#endif
