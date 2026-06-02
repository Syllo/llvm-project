#ifndef RIPPLE_DIV_F32_F16_HVX_FAST
#define RIPPLE_DIV_F32_F16_HVX_FAST

//===------------------- ripple_div_f32_f16_hvx_fast.h ----------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access a exp f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

// some utility functions are shared
#include "ripple_div_f32_f16_hvx.h"

//===------------------------------------------------------------------------===
// This header defines several internal implementations of vector div
// All functions are templated: you can choose to compile the overflow-handling
// code (safe=true) or not.
//
// As a rule, the overflow-handling code makes the function twice as slow.
//===------------------------------------------------------------------------===

// if we promoted from fp16, we don't need to check all cases; the only special
// case is 0.0f
template <bool safe = true>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_recip_fp32_fast(HVX_Vector vin) {
  const HVX_Vector inf = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector const253 = Q6_V_vsplat_R(253);
  const HVX_Vector constNeg1 = Q6_V_vsplat_R(0xFFFFFFFF);
  const HVX_Vector constNeg2 = Q6_V_vsplat_R(0xFFFFFFFE);
  const HVX_Vector bigSubnormal = Q6_V_vsplat_R(1 << 22);
  const HVX_Vector medSubnormal = Q6_V_vsplat_R(1 << 21);
  const HVX_Vector hiddenBit = Q6_V_vsplat_R(0x80000000);

  auto [u, exp, sign] = ripple_frexp_abs_fp32_hvx(vin);
  // can let go of the 'sign' vectorPred register
  HVX_Vector resSign = Q6_V_vmux_QVV(sign, hiddenBit, Q6_V_vzero());
  // compute the reciprocal with exponent 0
  u = ripple_compute_recip_approx_fp32<true>(u);
  // compute the new exponent; turn into a float
  exp = Q6_Vw_vsub_VwVw(const253, exp);
  // if exp is now == const25e, it was 0 before, so we're inverting a subnormal
  HVX_VectorPred return_inf, is_med_subnormal; // should compile out if !safe
  if constexpr (safe) {
    HVX_VectorPred input_subnormal = Q6_Q_vcmp_eq_VwVw(exp, const253);
    // if exp is now == -2 now, it was 255 before, so we're inveting inf. IEEE
    // says return an inf
    HVX_VectorPred input_inf = Q6_Q_vcmp_eq_VwVw(exp, constNeg2);
    return_inf = Q6_Q_or_QQ(input_inf, input_subnormal);
    // Unfortunately, there is one more special case: med-subnormall (input was
    // 2^127 scale)
    is_med_subnormal = Q6_Q_vcmp_eq_VwVw(exp, constNeg1);
  } else {
    // still need to check for 0.0f input, but nothing else.
    return_inf = Q6_Q_vcmp_eq_VwVw(exp, const253);
  }
  // left-shift into a float, power of 2
  exp = Q6_Vw_vasl_VwR(exp, 23);
  // if input subnormal, bump the multiplier up to inf
  exp = Q6_V_vmux_QVV(return_inf, inf, exp); // both safe and !safe computation
  // if output subnormal, don't go to 0, multiply with biggest subnormal value,
  // 2^-127 consider commenting this out; not super worth the computation
  if constexpr (safe) {
    HVX_VectorPred output_subnormal = Q6_Q_vcmp_eq_VwVw(exp, Q6_V_vzero());
    exp = Q6_V_vmux_QVV(output_subnormal, bigSubnormal, exp);
    // last special case, the med-subnormal
    exp = Q6_V_vmux_QVV(is_med_subnormal, medSubnormal, exp);
  }
  // multiply u and resulting exponent; return results to sf
  // this neatly handles results like 'the result is subnormal', etc
  u = _ripple_internal_Vsf_vmpy_VsfVsf(u, exp);
  return Q6_V_vor_VV(u, resSign);
}

RIPPLE_INTRIN_INLINE HVX_Vector
ripple_fp32_div_hvx_fast(HVX_Vector numerator, HVX_Vector denominator) {
  HVX_Vector recip = ripple_recip_fp32_fast(denominator);
  return _ripple_internal_Vsf_vmpy_VsfVsf(numerator, recip);
}

//==========================================================================
//==========================================================================

RIPPLE_INTRIN_INLINE HVX_Vector ripple_recip_fp16_promote_fast(HVX_Vector vin) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  HVX_VectorPair qfx = Q6_Wqf32_vmpy_VhfVhf(vin, const1);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_recip_fp32_fast<false>(Q6_Vsf_equals_Vqf32(Q6_V_hi_W(qfx))),
      Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_recip_fp32_fast<false>(Q6_Vsf_equals_Vqf32(Q6_V_lo_W(qfx))),
      Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vcombine_VV(hiRes, loRes);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

RIPPLE_INTRIN_INLINE HVX_Vector
ripple_div_fp16_promote_fast(HVX_Vector numerator, HVX_Vector denominator) {
  HVX_Vector recip = ripple_recip_fp16_promote_fast(denominator);
  return _ripple_internal_Vhf_vmpy_VhfVhf(numerator, recip);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_recip_fp16_fast(HVX_Vector vin) {
  const HVX_Vector inf = Q6_Vh_vsplat_R(0x7C00);
  const HVX_Vector const29 = Q6_Vh_vsplat_R(29);
  const HVX_Vector constNeg2 = Q6_Vh_vsplat_R(0xFFFE);
  const HVX_Vector constNeg1 = Q6_Vh_vsplat_R(0xFFFF);
  const HVX_Vector bigSubnormal = Q6_Vh_vsplat_R(1 << 9);
  const HVX_Vector medSubnormal = Q6_Vh_vsplat_R(1 << 8);
  const HVX_Vector hiddenBit = Q6_Vh_vsplat_R(0x8000);

  auto [u, exp, sign] = ripple_frexp_abs_fp16_hvx(vin);
  // can let go of the 'sign' vectorPred register
  HVX_Vector resSign = Q6_V_vmux_QVV(sign, hiddenBit, Q6_V_vzero());
  // compute the reciprocal with exponent 0
  u = ripple_compute_recip_approx_fp16<false>(u);
  // compute the new exponent; turn into a float
  exp = Q6_Vh_vsub_VhVh(const29, exp);
  // if exp is now == const29, it was 0 before, so we're inverting a subnormal
  HVX_VectorPred input_subnormal = Q6_Q_vcmp_eq_VhVh(exp, const29);
  // if exp is now == -2 now, it was 31 before, so we're inveting inf. IEEE says
  // return an inf
  HVX_VectorPred input_inf = Q6_Q_vcmp_eq_VhVh(exp, constNeg2);
  HVX_VectorPred return_inf = Q6_Q_or_QQ(input_inf, input_subnormal);
  // Unfortunately, there is one more special case: med-subnormall (input was
  // 2^15 scale)
  HVX_VectorPred is_med_subnormal = Q6_Q_vcmp_eq_VhVh(exp, constNeg1);
  // left-shift into a float, power of 2
  exp = Q6_Vh_vasl_VhR(exp, 10);
  // if input subnormal, bump the multiplier up to inf
  exp = Q6_V_vmux_QVV(return_inf, inf, exp);
  // if output subnormal, don't go to 0, multiply with biggest subnormal value,
  // 2^-15 consider commenting this out; not super worth the computation
  HVX_VectorPred output_subnormal = Q6_Q_vcmp_eq_VhVh(exp, Q6_V_vzero());
  exp = Q6_V_vmux_QVV(output_subnormal, bigSubnormal, exp);
  // last special case, the med-subnormal
  exp = Q6_V_vmux_QVV(is_med_subnormal, medSubnormal, exp);
  // multiply u and resulting exponent; return results to sf
  // this neatly handles results like 'the result is subnormal', etc
  u = _ripple_internal_Vhf_vmpy_VhfVhf(u, exp);
  return Q6_V_vor_VV(u, resSign);
}

RIPPLE_INTRIN_INLINE HVX_Vector
ripple_fp16_div_hvx_fast(HVX_Vector numerator, HVX_Vector denominator) {
  HVX_Vector recip = ripple_recip_fp16_fast(denominator);
  return Q6_Vsf_equals_Vqf32(Q6_Vqf16_vmpy_VhfVhf(numerator, recip));
}

#endif
