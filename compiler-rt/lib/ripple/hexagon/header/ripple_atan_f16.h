#ifndef RIPPLE_ATAN_F16_HEXAGON
#define RIPPLE_ATAN_F16_HEXAGON

//===-------------------------- ripple_atan_f16.h ---------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access a custom atan f16 vector implementation.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>

// ====================================================
// FUNCTION: 1/(x+1)     y(0) = 1,  y(0.5) = 0.6667, y(1) = 0.5
// Order:3; continuity: True; Ends forced: True
// Mode: unsigned;   Result fractional bits: 14
// Peak Error: 1.1295e-04  Rms Error: 2.8410e-05   Mean Error: 1.1370e-05
//      32769  -32706   31252  -10589
//      32590  -30635   22793   -4493
//      32066  -27505   16481   -2348
//      31205  -24054   11849   -1306

RIPPLE_INTRIN_INLINE HVX_Vector recip_xp1_O3_unsigned(HVX_Vector vx) {
  // input is 0..0xffff representing 0.0  .. 1.0
  HVX_Vector p;
  p = Q6_Vh_vlut4_VuhPh(vx, 0xFAE6F6D4EE73D6A3ull);
  p = Q6_Vh_vmpa_VhVhVuhPuh_sat(p, vx, 0x2E49406159097A14ull);
  p = Q6_Vh_vmps_VhVhVuhPuh_sat(p, vx, 0x5DF66B7177AB7FC2ull);
  p = Q6_Vh_vmpa_VhVhVuhPuh_sat(p, vx, 0x79E57D427F4E8001ull);
  return p; // signed result, 14 fractional bits
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_atanf16_hvx(HVX_Vector in) {
  const HVX_Vector sign_mask = Q6_Vh_vsplat_R(0x7FFF);
  const HVX_Vector exp_mask = Q6_Vh_vsplat_R(0x7C00);

  const HVX_Vector c00 = Q6_Vh_vsplat_R(0x8665);
  const HVX_Vector c01 = Q6_Vh_vsplat_R(0xA204);
  const HVX_Vector c10 = Q6_Vh_vsplat_R(0x3C0C);
  const HVX_Vector c11 = Q6_Vh_vsplat_R(0x3C6A);
  const HVX_Vector c20 = Q6_Vh_vsplat_R(0xB005);
  const HVX_Vector c21 = Q6_Vh_vsplat_R(0xB4E9);

  const HVX_Vector qhalf_pi =
      Q6_Vqf16_vadd_VhfVhf(Q6_Vh_vsplat_R(0x3E48), Q6_V_vzero());

  HVX_Vector sign_bit = Q6_V_vand_VV(in, Q6_Vh_vsplat_R(0x8000));

  HVX_Vector abs = Q6_V_vand_VV(in, sign_mask);
  HVX_Vector exp = Q6_Vuh_vlsr_VuhR(Q6_V_vand_VV(abs, exp_mask), 10);

  HVX_VectorPred ge_one = Q6_Q_vcmp_gt_VhVh(exp, Q6_Vh_vsplat_R(14));

  // get the reciprocal
  HVX_VectorPair to_qf32 =
      Q6_Wqf32_vmpy_VhfVhf(abs, Q6_Vh_vsplat_R(0x3C00)); // *1.0
  HVX_Vector to_f32_0 = Q6_Vsf_equals_Vqf32(Q6_V_lo_W(to_qf32));
  HVX_Vector to_f32_1 = Q6_Vsf_equals_Vqf32(Q6_V_hi_W(to_qf32));
  HVX_Vector mant_u16 = Q6_Vh_vshuffo_VhVh(Q6_Vw_vasl_VwR(to_f32_1, 9),
                                           Q6_Vw_vasl_VwR(to_f32_0, 9));
  HVX_Vector exp_u16 = Q6_Vh_vshuffo_VhVh(to_f32_1, to_f32_0);
  HVX_Vector rm = recip_xp1_O3_unsigned(mant_u16);
  HVX_Vector vcl0 = Q6_Vuh_vcl0_Vuh(rm); // count leading zeros
  // Get mantissa for 16-bit represenation
  HVX_Vector mant_recip = Q6_V_vand_VV(
      Q6_Vh_vasr_VhR(Q6_Vh_vasl_VhVh(rm, vcl0), 5), Q6_Vh_vsplat_R(0x03FF));
  // Get exponent in IEEE 32-bit representation
  exp_u16 = Q6_Vuh_vlsr_VuhR(exp_u16, 7);
  // Compute Reciprocal Exponent
  HVX_Vector exp_recip =
      Q6_Vh_vsub_VhVh(Q6_Vh_vsub_VhVh(Q6_Vh_vsplat_R(254), exp_u16),
                      Q6_Vh_vsub_VhVh(vcl0, Q6_Vh_vsplat_R(1)));
  // Convert it for 16-bit representation
  exp_recip = Q6_Vh_vadd_VhVh_sat(
      Q6_Vh_vsub_VhVh(exp_recip, Q6_Vh_vsplat_R(127)), Q6_Vh_vsplat_R(15));
  exp_recip = Q6_Vh_vasl_VhR(exp_recip, 10);

  // Merge exponent and mantissa for reciprocal
  HVX_Vector recip = Q6_V_vor_VV(exp_recip, mant_recip);

  abs = Q6_V_vmux_QVV(ge_one, recip, abs);

  // two segment ([0,0.25], (0.25,1]) approx with ~0.004 peak abs error
  HVX_VectorPred seg2 = Q6_Q_vcmp_gt_VhVh(abs, Q6_Vh_vsplat_R(0x3400));
  HVX_Vector c0 = Q6_V_vmux_QVV(seg2, c01, c00);
  HVX_Vector c1 = Q6_V_vmux_QVV(seg2, c11, c10);
  HVX_Vector c2 = Q6_V_vmux_QVV(seg2, c21, c20);

  HVX_Vector approx = Q6_Vqf16_vmpy_VhfVhf(abs, c2);
  approx = Q6_Vqf16_vadd_Vqf16Vhf(approx, c1);
  approx = Q6_Vqf16_vmpy_Vqf16Vhf(approx, abs);
  approx = Q6_Vqf16_vadd_Vqf16Vhf(approx, c0);

  // fixup for taking the recip
  HVX_Vector recip_fixup = Q6_Vqf16_vsub_Vqf16Vqf16(qhalf_pi, approx);
  approx = Q6_V_vmux_QVV(ge_one, recip_fixup, approx);
  approx = Q6_Vhf_equals_Vqf16(approx);

  // fixup for negatives
  approx = Q6_V_vor_VV(approx, sign_bit);

  return approx;
}

#endif
