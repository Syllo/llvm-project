#ifndef RIPPLE_DIV_F32_F16_HVX
#define RIPPLE_DIV_F32_F16_HVX

//===---------------------- ripple_div_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access a div f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

//===------------------------------------------------------------------------===
// This header defines several internal implementations of vector div
// All functions are templated: you can choose to compile the overflow-handling
// code (safe=true) or not.
//
// As a rule, the overflow-handling code makes the function twice as slow.
//===------------------------------------------------------------------------===

#ifdef __cplusplus
namespace {
#endif

struct ripple_internal_frexp_abs_results {
  HVX_Vector norm;
  HVX_Vector exp;
  HVX_Vector sign;
};

struct ripple_internal_subnormal_preprocess_results {
  HVX_VectorPred is_subnormal;
  HVX_Vector adjusted;
};

#ifdef __cplusplus
} // namespace
#endif

template <bool force_exp = true>
RIPPLE_INTRIN_INLINE ripple_internal_frexp_abs_results
ripple_frexp_abs_fp32_hvx(HVX_Vector vin) {
  // Perform the frexp operation and extract the sign
  // Return a vector of mantissas in range [0.5, 1.0)
  // And a vector of the original exponents
  // So we can reconstruct the original value just by doing
  // a bit wise or between the three returned vectors
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector mant_extract =
      Q6_V_vsplat_R(0x7FFFFF); // Extracts the mantissa
  const HVX_Vector sign_extract =
      Q6_V_vsplat_R(0x80000000); // extracts the sign
  const HVX_Vector fixed_exp =
      Q6_V_vsplat_R(0x3F000000); // used to force exp == 126
  HVX_Vector exp = Q6_V_vand_VV(exp_extract, vin);
  HVX_Vector norm = Q6_V_vand_VV(mant_extract, vin);
  HVX_Vector sign = Q6_V_vand_VV(sign_extract, vin);
  if constexpr (force_exp)
    norm = Q6_V_vor_VV(norm, fixed_exp);
  exp = Q6_Vuw_vlsr_VuwR(exp, 23); // shift exp to the low bits
  return {norm, exp, sign};
}

// alwasy use three_newton to avoid strange behavior
template <bool three_newton = true>
RIPPLE_INTRIN_INLINE HVX_Vector
ripple_compute_recip_approx_fp32(HVX_Vector vin) {
  const HVX_Vector lin_offset =
      Q6_V_vsplat_R(0x4034B4B5); // ~48/17 in IEEE FP32
  const HVX_Vector lin_scale =
      Q6_V_vsplat_R(0xBFF0F0F1); // ~ -32/17 in IEEE FP32
  HVX_Vector lin_approx = Q6_Vqf32_vmpy_VsfVsf(lin_scale, vin);
  lin_approx =
      Q6_Vqf32_vadd_Vqf32Vsf(lin_approx, lin_offset); // == 48/17 - 32/17*vin
  // lin_approx is our seed for the newtons method
  HVX_Vector neg_vin_qf = Q6_Vqf32_vmpy_VsfVsf(
      vin, Q6_V_vsplat_R(0xBF800000)); // well technically we need this
  const HVX_Vector v2 = Q6_V_vsplat_R(0x40000000); // 2.0f
  // Each iteration computes Xi(2-DXi)
  // Where D is vin, and Xi is the result of the previous iteration
  // Or the intial linear approximation
  HVX_Vector step0 =
      Q6_Vqf32_vmpy_Vqf32Vqf32(lin_approx, neg_vin_qf); // compute -DX0
  step0 = Q6_Vqf32_vadd_Vqf32Vsf(step0, v2);            // -DX0 + 2 == 2-DX0
  step0 = Q6_Vqf32_vmpy_Vqf32Vqf32(lin_approx, step0);  // X0(2-DX0)
  // Second iter
  HVX_Vector step1 =
      Q6_Vqf32_vmpy_Vqf32Vqf32(step0, neg_vin_qf); // compute -DX1
  step1 = Q6_Vqf32_vadd_Vqf32Vsf(step1, v2);
  step1 = Q6_Vqf32_vmpy_Vqf32Vqf32(step1, step0);
  if constexpr (three_newton) {
    // Third iter
    HVX_Vector step2 = Q6_Vqf32_vmpy_Vqf32Vqf32(step1, neg_vin_qf);
    step2 = Q6_Vqf32_vadd_Vqf32Vsf(step2, v2);
    step2 = Q6_Vqf32_vmpy_Vqf32Vqf32(step2, step1);
    return Q6_Vsf_equals_Vqf32(step2);
  } else {
    return step1;
  }
}

RIPPLE_INTRIN_INLINE ripple_internal_subnormal_preprocess_results
ripple_preprocess_subnormals_hvx_fp32(HVX_Vector vin) {
  const HVX_Vector subnormal_detect = Q6_V_vsplat_R(0x7F000000);
  HVX_Vector vin_subnormal = Q6_V_vand_VV(subnormal_detect, vin);
  HVX_VectorPred is_subnormal = Q6_Q_vcmp_eq_VwVw(vin_subnormal, Q6_V_vzero());
  const HVX_Vector subnormal_adj = Q6_V_vsplat_R(0x4B000000); // 2^23
  HVX_Vector adj = _ripple_internal_Vsf_vmpy_VsfVsf(vin, subnormal_adj);
  HVX_Vector res = Q6_V_vmux_QVV(is_subnormal, adj, vin);
  return {is_subnormal, res};
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_fp32_div_hvx(HVX_Vector numerator,
                                                    HVX_Vector denominator) {

  const HVX_Vector exp_bias = Q6_V_vsplat_R(127);
  const HVX_Vector max_exp = Q6_V_vsplat_R(255);
  const HVX_Vector max_mant = Q6_V_vsplat_R(0x7FFFFF);
  const HVX_Vector subnormalExp = Q6_V_vsplat_R(23);
  const HVX_Vector maxRightShift = Q6_V_vsplat_R(24);
  const HVX_Vector subnormalThreshold = Q6_V_vsplat_R(-126);
  const HVX_Vector hiddenBit = Q6_V_vsplat_R(0x800000);
  // detect subnormal in denominator
  auto [denominator_subnormal, adjustedDenominator] =
      ripple_preprocess_subnormals_hvx_fp32(denominator);
  // detect subnormal in numerator
  auto [numerator_subnormal, adjustedNumerator] =
      ripple_preprocess_subnormals_hvx_fp32(numerator);
  // Perform frexp and return the abs value + sign
  auto [normalizedDenominator, denomExp, denomSign] =
      ripple_frexp_abs_fp32_hvx(adjustedDenominator);
  auto [normalizedNumerator, numerExp, numerSign] =
      ripple_frexp_abs_fp32_hvx(adjustedNumerator);

  // Unbias the exponents
  denomExp = Q6_Vw_vsub_VwVw(denomExp, exp_bias);
  numerExp = Q6_Vw_vsub_VwVw(numerExp, exp_bias);

  // Start computing the result exp
  HVX_Vector expSum = Q6_Vw_vsub_VwVw(numerExp, denomExp);

  expSum = Q6_Vw_vadd_VwVw(
      expSum, Q6_V_vmux_QVV(denominator_subnormal, subnormalExp, Q6_V_vzero()));
  expSum = Q6_Vw_vsub_VwVw(
      expSum, Q6_V_vmux_QVV(numerator_subnormal, subnormalExp, Q6_V_vzero()));

  HVX_Vector resSign = Q6_V_vxor_VV(numerSign, denomSign);
  HVX_Vector recip = ripple_compute_recip_approx_fp32(normalizedDenominator);
  HVX_Vector normalizedRes =
      _ripple_internal_Vsf_vmpy_VsfVsf(normalizedNumerator, recip);
  // Extract mant, exp from the result
  // We won't actually need the sign because it is guaranteed to always be 0 but
  // oh well
  auto [resMant, resExp, resSignUnused] =
      ripple_frexp_abs_fp32_hvx<false>(normalizedRes);
  expSum = Q6_Vw_vsub_VwVw(expSum, exp_bias);
  expSum = Q6_Vw_vadd_VwVw(expSum, resExp);

  HVX_Vector final_exp = Q6_Vw_vadd_VwVw(expSum, exp_bias);

  HVX_VectorPred aboveMax = Q6_Q_vcmp_gt_VwVw(expSum, exp_bias);
  final_exp = Q6_V_vmux_QVV(aboveMax, max_exp, final_exp);
  resMant = Q6_V_vmux_QVV(aboveMax, max_mant, resMant);

  // if(expSum < -126) subnormal value
  HVX_VectorPred subnormalRes = Q6_Q_vcmp_gt_VwVw(subnormalThreshold, expSum);
  HVX_Vector shamt = Q6_Vw_vabs_Vw(Q6_Vw_vsub_VwVw(
      expSum, subnormalThreshold)); // subnormalThreshold is negative, do a
                                    // subtract to actually add it
  // HVX word shifts are limited to 5 bits, saturate if necessary
  shamt = Q6_Vw_vmin_VwVw(shamt, maxRightShift);
  HVX_Vector subnormalMant = Q6_V_vor_VV(hiddenBit, resMant);
  subnormalMant = Q6_Vw_vlsr_VwVw(subnormalMant, shamt);

  final_exp = Q6_V_vmux_QVV(subnormalRes, Q6_V_vzero(), final_exp);
  resMant = Q6_V_vmux_QVV(subnormalRes, subnormalMant, resMant);

  // create the final result
  final_exp = Q6_Vw_vasl_VwR(final_exp, 23);
  HVX_Vector res = Q6_V_vor_VV(resSign, resMant);
  res = Q6_V_vor_VV(res, final_exp);

  return res;
}

// note: not fast. Calls div under the hood
RIPPLE_INTRIN_INLINE HVX_Vector ripple_recip_fp32_div_hvx(HVX_Vector vin) {
  HVX_Vector ones = Q6_V_vsplat_R(0x3f800000);
  return ripple_fp32_div_hvx(ones, vin);
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

template <bool force_exp = true>
RIPPLE_INTRIN_INLINE ripple_internal_frexp_abs_results
ripple_frexp_abs_fp16_hvx(HVX_Vector vin) {
  // Perform the frexp operation and extract the sign
  // Return a vector of mantissas in range [0.5, 1.0)
  // And a vector of the original exponents
  // So we can reconstruct the original value just by doing
  // a bit wise or between the three returned vectors

  // the splated values are repeated twice because vsplat consumes a 4 byte word
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7C007C00);
  const HVX_Vector mant_extract =
      Q6_V_vsplat_R(0x03FF03FF); // Extracts the mantissa
  const HVX_Vector sign_extract =
      Q6_V_vsplat_R(0x80008000); // extracts the sign
  const HVX_Vector fixed_exp =
      Q6_V_vsplat_R(0x38003800); // used to force exp == 14
  HVX_Vector exp = Q6_V_vand_VV(exp_extract, vin);
  HVX_Vector norm = Q6_V_vand_VV(mant_extract, vin);
  HVX_Vector sign = Q6_V_vand_VV(sign_extract, vin);
  if constexpr (force_exp)
    norm = Q6_V_vor_VV(norm, fixed_exp);
  exp = Q6_Vuh_vlsr_VuhR(exp, 10); // shift exp to the low bits
  return {norm, exp, sign};
}

// experiments show three_newton actually reduces accuracy for f16
template <bool three_newton = false>
RIPPLE_INTRIN_INLINE HVX_Vector
ripple_compute_recip_approx_fp16(HVX_Vector vin) {
  const HVX_Vector lin_offset =
      Q6_V_vsplat_R(0x41A641A6); // ~48/17 in IEEE FP16
  const HVX_Vector lin_scale =
      Q6_V_vsplat_R(0xBF88BF88); // ~ -32/17 in IEEE FP16
  HVX_Vector lin_approx = Q6_Vqf16_vmpy_VhfVhf(lin_scale, vin);
  lin_approx =
      Q6_Vqf16_vadd_Vqf16Vhf(lin_approx, lin_offset); // == 48/17 - 32/17*vin
  // lin_approx is our seed for the newtons method
  HVX_Vector neg_vin_qf = Q6_Vqf16_vmpy_VhfVhf(
      vin, Q6_V_vsplat_R(0xBC00BC00)); // well technically we need this
  const HVX_Vector v2 = Q6_V_vsplat_R(0x40004000); // 2.0f
  // Each iteration computes Xi(2-DXi)
  // Where D is vin, and Xi is the result of the previous iteration
  // Or the intial linear approximation
  HVX_Vector step0 =
      Q6_Vqf16_vmpy_Vqf16Vqf16(lin_approx, neg_vin_qf); // compute -DX0
  step0 = Q6_Vqf16_vadd_Vqf16Vhf(step0, v2);            // -DX0 + 2 == 2-DX0
  step0 = Q6_Vqf16_vmpy_Vqf16Vqf16(lin_approx, step0);  // X0(2-DX0)
  // Second iter
  HVX_Vector step1 =
      Q6_Vqf16_vmpy_Vqf16Vqf16(step0, neg_vin_qf); // compute -DX1
  step1 = Q6_Vqf16_vadd_Vqf16Vhf(step1, v2);
  step1 = Q6_Vqf16_vmpy_Vqf16Vqf16(step1, step0);
  if constexpr (three_newton) {
    // Third iter; check if needed at atll
    HVX_Vector step2 = Q6_Vqf16_vmpy_Vqf16Vqf16(step1, neg_vin_qf);
    step2 = Q6_Vqf16_vadd_Vqf16Vhf(step2, v2);
    step2 = Q6_Vqf16_vmpy_Vqf16Vqf16(step2, step1);
    return Q6_Vhf_equals_Vqf16(step2);
  } else {
    return Q6_Vhf_equals_Vqf16(step1);
  }
}

#endif
