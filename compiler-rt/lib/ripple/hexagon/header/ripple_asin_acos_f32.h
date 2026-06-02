#ifndef RIPPLE_ASIN_ACOS_F32
#define RIPPLE_ASIN_ACOS_F32

//===------------------------ ripple_asin_acos_f32.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//  Internal header to access custom asin and acos f32 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include "ripple_qhmath_hvx.h"
#include "ripple_trunc_round_f32_f16.h"
#include <hexagon_types.h>

alignas(sizeof(HVX_Vector)) static const float ripple_asin_AB_vals[32] = {
    // As:
    1.0f, 0.9999999999928227f, 0.9999999994050468f, 0.9999999901399756f,
    0.9999999202194649f, 0.999999570968285f, 0.9999982251736466f,
    0.999993870700914f, 0.9999814061408046f, 0.9999487196215782f,
    0.9998681982281887f, 0.9996782213291908f, 0.9992420112163884f,
    0.9982531043100664f, 0.9960079101048365f, 0.9908231676781867f,
    // Bs:
    0.16666649591395608f, 0.16666667584752304f, 0.16666688623121217f,
    0.1666684122658353f, 0.16667498130357217f, 0.166696124407963f,
    0.16675291622198074f, 0.1668882295681442f, 0.1671852143729211f,
    0.16780116275742785f, 0.16903103034621836f, 0.1714301632898696f,
    0.17606029904663956f, 0.18500558229413855f, 0.20251765249440723f,
    0.23774237924135377f};

alignas(sizeof(HVX_Vector)) static const float ripple_asin_CD_vals[32] = {
    // Cs:
    0.07517473453394967f, 0.07499612655374222f, 0.07497115907608531f,
    0.07488748485775432f, 0.07468186220103235f, 0.07425530031287053f,
    0.0734565367038158f, 0.07205504341800406f, 0.06969648207359569f,
    0.0658276360486525f, 0.059566252004060516f, 0.04946733425049388f,
    0.03308559680272083f, 0.006114310707062032f, -0.03941489499311501f,
    -0.119184370284529f,
    // Ds:
    0.0f, 0.04524202272295952f, 0.04622360272333026f, 0.04775076772784814f,
    0.04989515268243849f, 0.052763124025659636f, 0.056507462744775694f,
    0.061345689126028446f, 0.06758900712702598f, 0.07568891019218427f,
    0.08631435409461119f, 0.10048410567469546f, 0.1198035367601733f,
    0.14691032054679454f, 0.18636618590358012f, 0.2465797584984557f};

// continuous function and continuous first derivative
RIPPLE_INTRIN_INLINE HVX_Vector ripple_asinf32_hvx(HVX_Vector xval) {
  // print_ALL_float(xval);

  // extract signbit and take absolute value
  const HVX_Vector sign_bit = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x80000000));
  xval = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x7FFFFFFF));
  // print_ALL_float(xval);

  // mark out-of-bounds
  const HVX_Vector const1 = Q6_V_vsplat_R(0x3f800000);
  const HVX_VectorPred outOfBounds = Q6_Q_vcmp_gt_VwVw(xval, const1);

  // cut to domain, at sqrt(0.5)
  // for values above sqrt(0.5), we will flag qbignum and compute
  // (pi/2) - asin(sqrt(1 - x^2))
  const HVX_Vector flip = qhmath_hvx_sqrt_vf(_ripple_internal_Vsf_vsub_VsfVsf(
      const1, _ripple_internal_Vsf_vmpy_VsfVsf(xval, xval)));
  const HVX_VectorPred bignum = Q6_Q_vcmp_gt_VwVw(xval, flip);
  xval = Q6_Vw_vmin_VwVw(xval, flip);
  // print_ALL_float(xval);

  // get index
  // 16 / sqrt(0.5)
  const HVX_Vector scaler = Q6_V_vsplat_R(0x41b504f3);
  const HVX_Vector int15 = Q6_V_vsplat_R(15);
  HVX_Vector inds = _ripple_internal_Vsf_vmpy_VsfVsf(xval, scaler);
  // sf -> w -> correct ind
  inds = Q6_Vw_vmin_VwVw(int15, _ripple_internal_w_from_sf(inds));
  // print_ALL_unsigned(inds);

  // look up vals
  const HVX_Vector AB_table = *(HVX_Vector const *)ripple_asin_AB_vals;
  const HVX_Vector CD_table = *(HVX_Vector const *)ripple_asin_CD_vals;

  HVX_Vector result =
      _ripple_internal_piecewise_poly_engine(xval, inds, AB_table, CD_table);

  // process bignum
  const HVX_Vector halfPi = Q6_V_vsplat_R(0x3fc90fdb);
  HVX_Vector alt_res = _ripple_internal_Vsf_vsub_VsfVsf(halfPi, result);
  result = Q6_V_vmux_QVV(bignum, alt_res, result);

  // add back the sign bit and return
  result = Q6_V_vor_VV(sign_bit, result);

  const HVX_Vector constNan = Q6_V_vsplat_R(0x7fc00000);
  return Q6_V_vmux_QVV(outOfBounds, constNan, result);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_acosf32_hvx(HVX_Vector invec) {
  const HVX_Vector halfPi = Q6_V_vsplat_R(0x3fc90fdb);
  return _ripple_internal_Vsf_vsub_VsfVsf(halfPi, ripple_asinf32_hvx(invec));
}

#endif
