#ifndef RIPPLE_SINH_COSH_F32_F16_HVX
#define RIPPLE_SINH_COSH_F32_F16_HVX

//===------------------- ripple_sinh_cosh_f32_f16_hvx.h ---------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//    Internal header for sinch and cosh f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <stdbool.h>

// for some util functions
#include "ripple_exp_f32_f16_hvx.h"
#include "ripple_internal_utils_hvx.h"

//===------------------------------------------------------------------------===
// This header defines several internal implementations of vector sinh and cosh
// The boolean arguments select overflow handling and approximation variants.
// Callers pass constants so unused paths are removed after inlining.
//
// As a rule, the overflow-handling code makes the function 20% slower.
//
// These functions are based on Schraudolph's algorithm, which is a good
// approximation to exp
//===------------------------------------------------------------------------===

// almost a copy expf32 up to a certain point
// changed the bias to automatically divide exp by 2:
// 1056964608 = 126 * (2 ^ 23), instead of 1065353216
RIPPLE_INTRIN_INLINE HVX_Vector ripple_coshf32_hvx(HVX_Vector x, bool safe) {
  const HVX_Vector mask007FFFFF = Q6_V_vsplat_R(0x007fffff);
  const HVX_Vector mask000000FF = Q6_V_vsplat_R(0x000000ff);
  const HVX_Vector const00800000 = Q6_V_vsplat_R(0x00800000);
  const HVX_Vector const126 = Q6_V_vsplat_R(126);
  const HVX_Vector const31 = Q6_V_vsplat_R(31);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const12102203 = Q6_V_vsplat_R(12102203 << 7);
  const HVX_Vector const1056964608 = Q6_V_vsplat_R(1056964608);

  const HVX_Vector constNeg7 = Q6_V_vsplat_R(0xFFFFFFF9);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector constMaxU = Q6_V_vsplat_R(
      0x407fffff); // 0x7F7FFFFF - 1056964608; 0x7F7FFFFF is max positive float

  // this gets compiled out when not used; only used with 'safe'
  HVX_VectorPred isInf;

  // fp32 = 1 sign + 8 exp + 23 mant = sxxx xxxx xmmm mmmm mmmm mmmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vw_vasr_VwR(x, 23), mask000000FF);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask007FFFFF), const00800000);

  // union { float f; int32_t i; } u;
  // u.i = (int32_t)(12102203.0f * x + 1065353216);

  // mant: perform multiply (mpye + mpyo on mant by 12102203.0f)
  HVX_Vector u = Q6_Vw_vmpye_VwVuh(mant, const12102203);
  u = Q6_Vw_vmpyoacc_VwVwVh_s1_rnd_sat_shift(u, mant, const12102203);

  // exp: compute and perform shift on u
  HVX_Vector shift = Q6_Vw_vmin_VwVw(Q6_Vw_vsub_VwVw(const126, exp), const31);
  if (safe) {
    isInf =
        Q6_Q_vcmp_gt_VwVw(constNeg7, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
  }
  u = Q6_Vw_vasr_VwVw(u, shift);
  if (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuwVuw(u, constMaxU);
    isInf =
        Q6_Q_or_QQ(isInf, overMax); // if it's positive and too large, go to inf
  }

  // almost copy of expf32 up tothis point
  // find the big and the small exponentials and add them together
  HVX_Vector usmall =
      Q6_Vw_vmax_VwVw(Q6_Vw_vsub_VwVw(const1056964608, u), const0);
  u = Q6_Vw_vadd_VwVw(const1056964608, u);

  u = _ripple_exp_helper_polynomial_f32(u);
  usmall = _ripple_exp_helper_polynomial_f32(usmall);
  u = _ripple_internal_Vsf_vadd_VsfVsf(u, usmall);

  // now, call on isInf:
  if (safe) {
    u = Q6_V_vmux_QVV(isInf, constInf, u);
  }

  return u;
}

// almost a copy expf32 up to a certain point
// changed the bias to automatically divide exp by 2:
// 1056964608 = 126 * (2 ^ 23), instead of 1065353216
RIPPLE_INTRIN_INLINE HVX_Vector ripple_sinhf32_hvx(HVX_Vector x, bool safe) {
  const HVX_Vector maskSignBit = Q6_V_vsplat_R(0x80000000);

  const HVX_Vector mask007FFFFF = Q6_V_vsplat_R(0x007fffff);
  const HVX_Vector mask000000FF = Q6_V_vsplat_R(0x000000ff);
  const HVX_Vector const00800000 = Q6_V_vsplat_R(0x00800000);
  const HVX_Vector const126 = Q6_V_vsplat_R(126);
  const HVX_Vector const31 = Q6_V_vsplat_R(31);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const12102203 = Q6_V_vsplat_R(12102203 << 7);
  const HVX_Vector const1056964608 = Q6_V_vsplat_R(1056964608);

  const HVX_Vector constNeg7 = Q6_V_vsplat_R(0xFFFFFFF9);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector constMaxU = Q6_V_vsplat_R(
      0x407fffff); // 0x7F7FFFFF - 1056964608; 0x7F7FFFFF is max positive float

  // this gets compiled out when not used; only used with 'safe'
  HVX_VectorPred isInf;

  // fp32 = 1 sign + 8 exp + 23 mant = sxxx xxxx xmmm mmmm mmmm mmmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vw_vasr_VwR(x, 23), mask000000FF);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask007FFFFF), const00800000);
  HVX_VectorPred sign = Q6_Q_vcmp_gt_VwVw(const0, x);

  // union { float f; int32_t i; } u;
  // u.i = (int32_t)(12102203.0f * x + 1065353216);

  // mant: perform multiply (mpye + mpyo on mant by 12102203.0f)
  HVX_Vector u = Q6_Vw_vmpye_VwVuh(mant, const12102203);
  u = Q6_Vw_vmpyoacc_VwVwVh_s1_rnd_sat_shift(u, mant, const12102203);

  // exp: compute and perform shift on u
  HVX_Vector shift = Q6_Vw_vmin_VwVw(Q6_Vw_vsub_VwVw(const126, exp), const31);
  if (safe) {
    isInf =
        Q6_Q_vcmp_gt_VwVw(constNeg7, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
  }
  u = Q6_Vw_vasr_VwVw(u, shift);
  if (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuwVuw(u, constMaxU);
    isInf = Q6_Q_or_QQ(
        isInf, overMax); // if it's too large, go to inf; sign corrected later
  }

  // almost copy of expf32 up tothis point
  // find the big and the small exponentials and add them together
  HVX_Vector usmall =
      Q6_Vw_vmax_VwVw(Q6_Vw_vsub_VwVw(const1056964608, u), const0);
  u = Q6_Vw_vadd_VwVw(const1056964608, u);

  u = _ripple_exp_helper_polynomial_f32(u);
  usmall = _ripple_exp_helper_polynomial_f32(usmall);
  u = _ripple_internal_Vsf_vsub_VsfVsf(u, usmall);

  // now, call on isInf:
  if (safe) {
    u = Q6_V_vmux_QVV(isInf, constInf, u);
  }

  HVX_Vector uNeg = Q6_V_vor_VV(u, maskSignBit);

  return Q6_V_vmux_QVV(sign, uNeg, u);
}

//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_coshf16_hvx_promote(HVX_Vector x,
                                                           bool safe) {
  HVX_VectorPair sfx = _ripple_internal_Wsf_vcvt_Vhf(x);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_coshf32_hvx(Q6_V_hi_W(sfx), safe), Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_coshf32_hvx(Q6_V_lo_W(sfx), safe), Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vcombine_VV(hiRes, loRes);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_sinhf16_hvx_promote(HVX_Vector x,
                                                           bool safe) {
  HVX_VectorPair sfx = _ripple_internal_Wsf_vcvt_Vhf(x);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_sinhf32_hvx(Q6_V_hi_W(sfx), safe), Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_sinhf32_hvx(Q6_V_lo_W(sfx), safe), Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vcombine_VV(hiRes, loRes);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

// almost a copy expf16 up to a certain point
// changed the bias to automatically divide exp by 2:
// 14336 = 14 * (2 ^ 10), instead of 15360
RIPPLE_INTRIN_INLINE HVX_Vector ripple_coshf16_hvx(HVX_Vector x, bool safe) {
  // the splated values are repeated twice because vsplat consumes a 4 byte word
  const HVX_Vector mask03FF = Q6_V_vsplat_R(0x03FF03FF);
  const HVX_Vector mask001F = Q6_V_vsplat_R(0x001F001F);
  //    const HVX_Vector mask00FF = Q6_V_vsplat_R(0x00FF00FF);
  const HVX_Vector const0400 = Q6_V_vsplat_R(0x04000400);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1477 = Q6_V_vsplat_R(
      (1477 << 5) + (1477 << 21)); // left shifted as far it as can go; still
                                   // lose 1 bit of accuracy
  const HVX_Vector const14336 = Q6_V_vsplat_R(14336 + (14336 << 16));
  const HVX_Vector const14 = Q6_V_vsplat_R(0x000E000E);

  const HVX_Vector constNeg4 = Q6_V_vsplat_R(0xFFFCFFFC);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7C007C00);
  const HVX_Vector constMaxU =
      Q6_V_vsplat_R(0x43FF43FF); // 0x7BFF - 14336; 0x7BFF is max positive fp16

  // compiled out if 'safe' flag not set
  HVX_VectorPred isInf;

  // fp16 = 1 sign + 5 exp + 10 mant = sxxx xxmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vh_vasr_VhR(x, 10), mask001F);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask03FF), const0400);

  // union { float16 f; int16_t i; } u;
  // u.i = (int32_t)(1477.0f * x + 15360);

  // mant: perform multiply
  HVX_VectorPair temp = Q6_Wuw_vmpy_VuhVuh(
      mant, const1477); // 16bit * 11bit = 27bit; want the 27th bit to be the
                        // 11th, or 27th in word; just shift right
  HVX_Vector u = Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(temp), Q6_V_lo_W(temp)); // >>16

  // exp: compute and perform shift on u
  HVX_Vector shift = Q6_Vh_vsub_VhVh(const14, exp);

  if (safe) {
    isInf =
        Q6_Q_vcmp_gt_VhVh(constNeg4, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
  }
  u = Q6_Vh_vasr_VhVh(u, shift);
  if (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuhVuh(u, constMaxU);
    isInf =
        Q6_Q_or_QQ(isInf, overMax); // if it's positive and too large, go to inf
  }

  // get the big and small exp value
  HVX_Vector usmall = Q6_Vh_vmax_VhVh(Q6_Vh_vsub_VhVh(const14336, u), const0);
  u = Q6_Vh_vadd_VhVh(const14336, u);

  // almost a copy of expf16 up to this point

  // let go of all the vector predicate registers; Inf and 0 are unaffected by
  // the polynomial
  if (safe) {
    u = Q6_V_vmux_QVV(isInf, constInf, u);
  }
  u = _ripple_exp_helper_polynomial_f16(u);
  usmall = _ripple_exp_helper_polynomial_f16(usmall);

  return _ripple_internal_Vhf_vadd_VhfVhf(u, usmall);
}

// almost a copy expf16 up to a certain point
RIPPLE_INTRIN_INLINE HVX_Vector ripple_sinhf16_hvx(HVX_Vector x, bool safe) {
  const HVX_Vector maskSignBit = Q6_V_vsplat_R(0x80008000);

  // the splated values are repeated twice because vsplat consumes a 4 byte word
  const HVX_Vector mask03FF = Q6_V_vsplat_R(0x03FF03FF);
  const HVX_Vector mask001F = Q6_V_vsplat_R(0x001F001F);
  //    const HVX_Vector mask00FF = Q6_V_vsplat_R(0x00FF00FF);
  const HVX_Vector const0400 = Q6_V_vsplat_R(0x04000400);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1477 = Q6_V_vsplat_R(
      (1477 << 5) + (1477 << 21)); // left shifted as far it as can go; still
                                   // lose 1 bit of accuracy
  const HVX_Vector const14336 = Q6_V_vsplat_R(14336 + (14336 << 16));
  const HVX_Vector const14 = Q6_V_vsplat_R(0x000E000E);

  const HVX_Vector constNeg4 = Q6_V_vsplat_R(0xFFFCFFFC);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7C007C00);
  const HVX_Vector constMaxU =
      Q6_V_vsplat_R(0x43FF43FF); // 0x7BFF - 14336; 0x7BFF is max positive fp16

  // compiled out if 'safe' flag not set
  HVX_VectorPred isInf;

  // fp16 = 1 sign + 5 exp + 10 mant = sxxx xxmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vh_vasr_VhR(x, 10), mask001F);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask03FF), const0400);
  HVX_VectorPred sign = Q6_Q_vcmp_gt_VhVh(const0, x);

  // union { float16 f; int16_t i; } u;
  // u.i = (int32_t)(1477.0f * x + 15360);

  // mant: perform multiply
  HVX_VectorPair temp = Q6_Wuw_vmpy_VuhVuh(
      mant, const1477); // 16bit * 11bit = 27bit; want the 27th bit to be the
                        // 11th, or 27th in word; just shift right
  HVX_Vector u = Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(temp), Q6_V_lo_W(temp)); // >>16

  // exp: compute and perform shift on u
  HVX_Vector shift = Q6_Vh_vsub_VhVh(const14, exp);

  if (safe) {
    isInf =
        Q6_Q_vcmp_gt_VhVh(constNeg4, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
  }
  u = Q6_Vh_vasr_VhVh(u, shift);
  if (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuhVuh(u, constMaxU);
    isInf = Q6_Q_or_QQ(
        isInf, overMax); // if it's too large, go to inf; sign corrected later
  }

  // get the big and small exp value
  HVX_Vector usmall = Q6_Vh_vmax_VhVh(Q6_Vh_vsub_VhVh(const14336, u), const0);
  u = Q6_Vh_vadd_VhVh(const14336, u);

  // almost a copy of expf16 up to this point

  // let go of all the vector predicate registers; Inf and 0 are unaffected by
  // the polynomial
  if (safe) {
    u = Q6_V_vmux_QVV(isInf, constInf, u);
  }
  u = _ripple_exp_helper_polynomial_f16(u);
  usmall = _ripple_exp_helper_polynomial_f16(usmall);

  u = _ripple_internal_Vhf_vsub_VhfVhf(u, usmall);
  HVX_Vector uNeg = Q6_V_vor_VV(u, maskSignBit);
  return Q6_V_vmux_QVV(sign, uNeg, u);
}

#endif
