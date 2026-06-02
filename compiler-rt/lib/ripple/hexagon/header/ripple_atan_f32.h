#ifndef RIPPLE_ATAN_F32_HEXAGON
#define RIPPLE_ATAN_F32_HEXAGON

//===-------------------------- ripple_atan_f32.h ---------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access a custom atan f32 vector implementation.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_div_f32_f16_hvx_fast.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_qhmath_hvx.h"
#include "ripple_trunc_round_f32_f16.h"
#include <hexagon_types.h>

alignas(sizeof(HVX_Vector)) static const float ripple_atan_AB_vals[32] = {
    // As:
    1.0f, 0.9999999995990194f, 0.9999999691112311f, 0.9999995410159839f,
    0.9999967833022474f, 0.9999855076356718f, 0.9999513517527436f,
    0.9998677948689529f, 0.999693640448271f, 0.9993730980297499f,
    0.9988387011540212f, 0.9980164116158079f, 0.9968317481541931f,
    0.9952157305619842f, 0.9931097222889811f, 0.9904686846886044f,
    // Bs:
    -0.3333311666885664f, -0.33333307637855114f, -0.3333275971407943f,
    -0.3332921612299913f, -0.3331619119952052f, -0.3328187093944166f,
    -0.33209401259094307f, -0.33078857224487024f, -0.32870245533561615f,
    -0.325665885346325f, -0.32156278869408084f, -0.3163428494365803f,
    -0.3100219539428224f, -0.3026736743121887f, -0.2944155254486418f,
    -0.2853935090322466f};

alignas(sizeof(HVX_Vector)) static const float ripple_atan_CD_vals[32] = {
    // Cs:
    0.1988889932472469f, 0.19994559037149884f, 0.1996188819193776f,
    0.19864207047430682f, 0.19659201915237645f, 0.19311024786020425f,
    0.18798502282220397f, 0.18118663235554777f, 0.17285710926330466f,
    0.16326841443418516f, 0.1527672296675746f, 0.14172174266155935f,
    0.13047981224559635f, 0.11934189853718635f, 0.10854786271409012f,
    0.09827453588809476f,
    // Ds:
    0.0f, -0.13860809290781617f, -0.1321495182637591f, -0.12318318361212732f,
    -0.11243057093815878f, -0.10065747922635637f, -0.08857558154750222f,
    -0.07677429299928917f, -0.06568818826076495f, -0.055595331423489824f,
    -0.046636660828966114f, -0.03884584344848463f, -0.03218112446286625f,
    -0.026553817488917275f, -0.021850941612299835f, -0.017951548147493668f};

// continuous function and continuous first derivative
RIPPLE_INTRIN_INLINE HVX_Vector ripple_atanf32_hvx(HVX_Vector xval) {
  // print_ALL_float(xval);

  // extract signbit and take absolute value
  const HVX_Vector sign_bit = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x80000000));
  xval = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x7FFFFFFF));
  // print_ALL_float(xval);

  // cut to domain, at 1.0f
  // for values larger, we compute
  // (pi / 2) - atan(1 / x)
  const HVX_Vector rem = ripple_recip_fp32_fast(xval);
  const HVX_VectorPred bignum = Q6_Q_vcmp_gt_VwVw(xval, rem);
  xval = Q6_Vw_vmin_VwVw(xval, rem);
  // print_ALL_float(xval);

  // get index
  // 16
  const HVX_Vector const16 = Q6_V_vsplat_R(0x41800000);
  const HVX_Vector int15 = Q6_V_vsplat_R(15);
  HVX_Vector inds = _ripple_internal_Vsf_vmpy_VsfVsf(xval, const16);
  // sf -> w -> correct ind
  inds = Q6_Vw_vmin_VwVw(int15, _ripple_internal_w_from_sf(inds));
  // print_ALL_unsigned(inds);

  // look up vals
  const HVX_Vector AB_table = *(HVX_Vector const *)ripple_atan_AB_vals;
  const HVX_Vector CD_table = *(HVX_Vector const *)ripple_atan_CD_vals;

  HVX_Vector result =
      _ripple_internal_piecewise_poly_engine(xval, inds, AB_table, CD_table);

  // process bignum
  const HVX_Vector halfPi = Q6_V_vsplat_R(0x3fc90fdb);
  HVX_Vector alt_res = _ripple_internal_Vsf_vsub_VsfVsf(halfPi, result);
  result = Q6_V_vmux_QVV(bignum, alt_res, result);

  // add back the sign bit and return
  return Q6_V_vor_VV(sign_bit, result);
}

#endif
