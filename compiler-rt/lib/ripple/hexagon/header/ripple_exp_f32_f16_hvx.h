#ifndef RIPPLE_EXP_F32_F16_HVX
#define RIPPLE_EXP_F32_F16_HVX

//===---------------------- ripple_exp_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access a exp f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

//===------------------------------------------------------------------------===
// This header defines several internal implementations of vector exp
// All functions are templated: you can choose to compile the overflow-handling
// code (safe=true) or not.
//
// As a rule, the overflow-handling code makes the function twice as slow.
//
// Two of the functions are based on Schraudolph's algorithm, which is a good
// approximation to exp One of the fp16 functions is based on just promoting
// fp16 to fp32, then calling the Schraudolph algorithm (It would be wasteful to
// call the fully precise function for that: the accuracy would be lost when the
// results are demoted back down to fp16).
//
// Since these are backend functions, we include the algorithm name. More may be
// added later. These vectorized functions are called from HVX_MathLib.h, from
// functions with more user-friendly names like exp_vf16_fast. See that file to
// see which algorithm is used where.
//===------------------------------------------------------------------------===

// helper polynomial for f32 exp
// Cubic correction for mantissa
// m = u.i >> 7;
// u.i += ((((((((1277 * m) >> 7) + 14825) * m)) - 79749) * m)) - 626;
RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_exp_helper_polynomial_f32(HVX_Vector u) {
  const HVX_Vector const14825 = Q6_V_vsplat_R(14825 << 7);
  const HVX_Vector const79749 = Q6_V_vsplat_R(79749 << 5);
  const HVX_Vector const626 = Q6_V_vsplat_R(626);

  HVX_Vector m = Q6_Vw_vasr_VwR(u, 7);
  HVX_Vector cm =
      Q6_Vw_vasracc_VwVwR(const14825, Q6_Vuw_vmpye_VuhRuh(m, 1277), 7);
  cm = Q6_Vw_vsub_VwVw(Q6_Vw_vmpye_VwVuh(cm, m), const79749);
  cm = Q6_Vw_vsub_VwVw(Q6_Vw_vmpye_VwVuh(cm, m), const626);
  return Q6_Vw_vadd_VwVw(u, cm);
}

// f32 algorithm for exp. Based on Schraudolph's algorithm
//  Magic numbers:
//  12102203.0f ~= 2^23 / ln(2)
//  1065353216 ~= 127 * (2^23)
template <bool safe>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_expf32_hvx_schraudolph(HVX_Vector x) {
  const HVX_Vector mask007FFFFF = Q6_V_vsplat_R(0x007fffff);
  const HVX_Vector mask000000FF = Q6_V_vsplat_R(0x000000ff);
  const HVX_Vector const00800000 = Q6_V_vsplat_R(0x00800000);
  const HVX_Vector const126 = Q6_V_vsplat_R(126);
  const HVX_Vector const31 = Q6_V_vsplat_R(31);
  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const12102203 = Q6_V_vsplat_R(12102203 << 7);
  const HVX_Vector const1065353216 = Q6_V_vsplat_R(1065353216);

  const HVX_Vector constNeg7 = Q6_V_vsplat_R(0xFFFFFFF9);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7F800000);
  const HVX_Vector constMaxU = Q6_V_vsplat_R(
      0x3fffffff); // 0x7F7FFFFF - 1065353216; 0x7F7FFFFF is max positive float

  // fp32 = 1 sign + 8 exp + 23 mant = sxxx xxxx xmmm mmmm mmmm mmmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vw_vasr_VwR(x, 23), mask000000FF);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask007FFFFF), const00800000);
  HVX_VectorPred sign = Q6_Q_vcmp_gt_VwVw(const0, x);

  // union { float f; int32_t i; } u;
  // u.i = (int32_t)(12102203.0f * x + 1065353216);

  // mant: perform multiply (mpye + mpyo on mant by 12102203.0f)
  HVX_Vector u = Q6_Vw_vmpye_VwVuh(mant, const12102203);
  u = Q6_Vw_vmpyoacc_VwVwVh_s1_rnd_sat_shift(u, mant, const12102203);

  // exp: compute shift for u
  HVX_Vector shift = Q6_Vw_vmin_VwVw(Q6_Vw_vsub_VwVw(const126, exp), const31);

  // this gets compiled out when not used; only used with 'safe'
  HVX_VectorPred overflow;
  HVX_VectorPred isZero;
  HVX_VectorPred isInf;
  if constexpr (safe) {
    overflow =
        Q6_Q_vcmp_gt_VwVw(constNeg7, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
    isZero = Q6_Q_and_QQ(overflow, sign);    // negative overflows go to 0
    isInf = Q6_Q_and_QQn(overflow, sign);    // positive overflows go to inf
  }

  // compute the shift, then some more safety code
  u = Q6_Vw_vasr_VwVw(u, shift);
  if constexpr (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuwVuw(u, constMaxU);
    isInf = Q6_Q_or_QQ(
        isInf, Q6_Q_and_QQn(overMax,
                            sign)); // if it's positive and too large, go to inf
  }
  // sign: u computed so far does not account for sign
  //       thus, subtract from 1065353216 if negative
  HVX_Vector utemp0 = Q6_Vw_vadd_VwVw(const1065353216, u);
  HVX_Vector utemp1 = Q6_Vw_vsub_VwVw(const1065353216, u);
  u = Q6_V_vmux_QVV(sign, utemp1, utemp0);

  // we can let go of one vector predicate register; 0 is unaffected by the
  // polynomial
  if constexpr (safe)
    u = Q6_V_vmux_QVV(isZero, const0, u);

  u = _ripple_exp_helper_polynomial_f32(u);

  // now, call on isInf:
  if constexpr (safe)
    u = Q6_V_vmux_QVV(isInf, constInf, u);

  return u;
}

// works by turning the fp16 into an fp32 and calling the ripple fp32 function,
// above
template <bool safe>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_expf16_hvx_promote(HVX_Vector x) {
  HVX_VectorPair sfx = _ripple_internal_Wsf_vcvt_Vhf(x);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_expf32_hvx_schraudolph<safe>(Q6_V_hi_W(sfx)), Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_expf32_hvx_schraudolph<safe>(Q6_V_lo_W(sfx)), Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vcombine_VV(hiRes, loRes);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

// polynomial correction to the mantissa
// between 0 and 1, (2^x) - 1 - x ~ (0.25 / 0.0857864376) * x * (1 - x) ~
// 0.34*x*(1 - x) In hexadecimal, the expansion of the constant looks like
// 0.57d86660
RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_exp_helper_polynomial_f16(HVX_Vector u) {
  const HVX_Vector mask03FF = Q6_V_vsplat_R(0x03FF03FF);
  const HVX_Vector antimask03FF = Q6_V_vsplat_R(0xFC00FC00);
  const HVX_Vector const0058 = Q6_V_vsplat_R(0x00580058);
  const HVX_Vector const0100 = Q6_V_vsplat_R(0x01000100);
  const HVX_Vector const0 = Q6_V_vzero();

  HVX_Vector nm = Q6_V_vand_VV(u, antimask03FF); // store non-mantissa
  u = Q6_V_vand_VV(u, mask03FF);                 // get mantissa

  HVX_Vector m = Q6_Vh_vasr_VhR(u, 2);                    // 8 bit
  m = Q6_Vh_vmpyi_VhVh(Q6_Vh_vsub_VhVh(const0100, m), m); // m = m * (1 - m)
  m = Q6_Vh_vasr_VhR(
      m, 6); // >> 6; m is now 8 bit; maxval is 0.25, represented 0b10000000
  m = Q6_Vh_vmpyi_VhVh(m, const0058); // multiply by max correction
  m = Q6_Vh_vasr_VhR(m, 8); // 0x58 is actually the mantissa of the correction
                            // at 0.5; we move out the 8 bits of '0.25'

  u = Q6_Vh_vmax_VhVh(Q6_Vh_vsub_VhVh(u, m), const0);

  u = Q6_V_vor_VV(nm, u);

  return u;
}

// fp16 algorithm for exp. Based on Schraudolph's algorithm
// Magic numbers:
// 1477.0f ~= 2^10 / ln(2)
// 15360 = 15 * (2^10)
template <bool safe>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_expf16_hvx_schraudolph(HVX_Vector x) {
  // the splated values are repeated twice because vsplat consumes a 4 byte word
  const HVX_Vector mask03FF = Q6_V_vsplat_R(0x03FF03FF);
  const HVX_Vector mask001F = Q6_V_vsplat_R(0x001F001F);
  const HVX_Vector const0400 = Q6_V_vsplat_R(0x04000400);
  const HVX_Vector const0 = Q6_V_vzero();
  // left shifted as far it as can go
  const HVX_Vector const1477 = Q6_V_vsplat_R((1477 << 5) + (1477 << 21));
  const HVX_Vector const15360 = Q6_V_vsplat_R(15360 + (15360 << 16));
  const HVX_Vector const14 = Q6_V_vsplat_R(0x000E000E);

  const HVX_Vector constNeg4 = Q6_V_vsplat_R(0xFFFCFFFC);
  const HVX_Vector constInf = Q6_V_vsplat_R(0x7C007C00);
  const HVX_Vector constMaxU =
      Q6_V_vsplat_R(0x3FFF3FFF); // 0x7BFF - 15360; 0x7BFF is max positive fp16

  // fp16 = 1 sign + 5 exp + 10 mant = sxxx xxmm mmmm mmmm
  HVX_Vector exp = Q6_V_vand_VV(Q6_Vh_vasr_VhR(x, 10), mask001F);
  HVX_Vector mant = Q6_V_vor_VV(Q6_V_vand_VV(x, mask03FF), const0400);
  HVX_VectorPred sign = Q6_Q_vcmp_gt_VhVh(const0, x);

  // union { float16 f; int16_t i; } u;
  // u.i = (int32_t)(1477.0f * x + 15360);

  // mant: perform multiply
  HVX_VectorPair temp = Q6_Wuw_vmpy_VuhVuh(mant, const1477);
  HVX_Vector u = Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(temp), Q6_V_lo_W(temp)); // >>16

  // exp: compute shift for u
  HVX_Vector shift = Q6_Vh_vsub_VhVh(const14, exp);

  // compiled out if 'safe' flag not set
  HVX_VectorPred overflow;
  HVX_VectorPred isZero;
  HVX_VectorPred isInf;
  if constexpr (safe) {
    overflow =
        Q6_Q_vcmp_gt_VhVh(constNeg4, shift); // if we had to shift too far left,
                                             // we are overflowing the exponent
    isZero = Q6_Q_and_QQ(overflow, sign);    // negative overflows go to 0
    isInf = Q6_Q_and_QQn(overflow, sign);    // positive overflows go to inf
  }

  // perform shift, then additional safety code
  u = Q6_Vh_vasr_VhVh(u, shift);
  if constexpr (safe) {
    HVX_VectorPred overMax = Q6_Q_vcmp_gt_VuhVuh(u, constMaxU);
    isInf = Q6_Q_or_QQ(
        isInf, Q6_Q_and_QQn(overMax,
                            sign)); // if it's positive and too large, go to inf
  }
  // sign: u computed so far does not account for sign
  //       thus, subtract from 15360 if negative
  HVX_Vector utemp0 = Q6_Vh_vadd_VhVh(const15360, u);
  HVX_Vector utemp1 = Q6_Vh_vmax_VhVh(Q6_Vh_vsub_VhVh(const15360, u), const0);
  u = Q6_V_vmux_QVV(sign, utemp1, utemp0);

  // let go of all the vector predicate registers; Inf and 0 are unaffected by
  // the polynomial
  if constexpr (safe) {
    u = Q6_V_vmux_QVV(isZero, const0, u);
    u = Q6_V_vmux_QVV(isInf, constInf, u);
  }

  return _ripple_exp_helper_polynomial_f16(u);
}

#endif
