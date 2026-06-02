#ifndef RIPPLE_TANHF32_HVX
#define RIPPLE_TANHF32_HVX

//===-------------------------- ripple_tanh_f32.h ---------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access a custom tanh f32 vector implementation.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

#include "ripple_div_f32_f16_hvx.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_trunc_round_f32_f16.h"

// NaN and Inf go to 1, -Inf goes to -1
// Rational approximation
// continuous function
// continuous first derivative
// NOT monotonic (very small violations after |x| > 5
// RMS error 8.699898e-08
// Max error 3.161979e-07
RIPPLE_INTRIN_INLINE HVX_Vector ripple_tanh_f32_hvx(HVX_Vector input) {
  // 7.8522667885e+00f
  const HVX_Vector domain_max = Q6_V_vsplat_R(0x40fb45c5);
  const HVX_Vector sign_mask = Q6_V_vsplat_R(0x80000000);

  HVX_Vector sign_bit = Q6_V_vand_VV(input, sign_mask);
  input = Q6_V_vand_VV(input, Q6_V_vnot_V(sign_mask));
  input = Q6_Vw_vmin_VwVw(input, domain_max);

  const HVX_Vector const0 = Q6_V_vzero();
  input = Q6_Vqf32_vadd_VsfVsf(input, const0);
  HVX_Vector inp_squared = Q6_Vqf32_vmpy_Vqf32Vqf32(input, input);

  // coefficients of the numerator polynomial
  // 1.0f
  const HVX_Vector alpha1 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3f800000));
  // 1.3412411511e-01
  const HVX_Vector alpha3 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3e0957d5));
  // 3.5330520477e-03
  const HVX_Vector alpha5 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3b678ac7));
  // 2.1235626264e-05
  const HVX_Vector alpha7 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x37b22329));
  // 1.4248920266e-08
  const HVX_Vector alpha9 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3274cb6a));

  // coefficients of the denominator polynomial
  // 1.0
  const HVX_Vector beta0 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3f800000));
  // 4.6745735407e-01
  const HVX_Vector beta2 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3eef5692));
  // 2.6018999517e-02
  const HVX_Vector beta4 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x3cd525cc));
  // 3.3472978976e-04
  const HVX_Vector beta6 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x39af7eac));
  // 8.1365948290e-07
  const HVX_Vector beta8 =
      Q6_Vqf32_vadd_VsfVsf(const0, Q6_V_vsplat_R(0x355a6a41));

  // unfortunately, multiply-accumulate is not available for sf nor qf32,
  // so we are stuck with multiple operations
  HVX_Vector numerator = Q6_Vqf32_vadd_Vqf32Vqf32(
      alpha7, Q6_Vqf32_vmpy_Vqf32Vqf32(alpha9, inp_squared));
  numerator = Q6_Vqf32_vadd_Vqf32Vqf32(
      alpha5, Q6_Vqf32_vmpy_Vqf32Vqf32(numerator, inp_squared));
  numerator = Q6_Vqf32_vadd_Vqf32Vqf32(
      alpha3, Q6_Vqf32_vmpy_Vqf32Vqf32(numerator, inp_squared));
  numerator = Q6_Vqf32_vadd_Vqf32Vqf32(
      alpha1, Q6_Vqf32_vmpy_Vqf32Vqf32(numerator, inp_squared));
  numerator = Q6_Vqf32_vmpy_Vqf32Vqf32(numerator, input);

  HVX_Vector denominator = Q6_Vqf32_vadd_Vqf32Vqf32(
      beta6, Q6_Vqf32_vmpy_Vqf32Vqf32(beta8, inp_squared));
  denominator = Q6_Vqf32_vadd_Vqf32Vqf32(
      beta4, Q6_Vqf32_vmpy_Vqf32Vqf32(denominator, inp_squared));
  denominator = Q6_Vqf32_vadd_Vqf32Vqf32(
      beta2, Q6_Vqf32_vmpy_Vqf32Vqf32(denominator, inp_squared));
  denominator = Q6_Vqf32_vadd_Vqf32Vqf32(
      beta0, Q6_Vqf32_vmpy_Vqf32Vqf32(denominator, inp_squared));

  // Now, since the div algorithms take sf, not qf32, we have to convert back
  // and add in the sign bit.
  HVX_Vector num_sf = Q6_V_vor_VV(Q6_Vsf_equals_Vqf32(numerator), sign_bit);
  HVX_Vector den_sf = Q6_Vsf_equals_Vqf32(denominator);

  // now call the best div func
  return ripple_fp32_div_hvx(num_sf, den_sf);
}

#endif
