#ifndef RIPPLE_TANHF32_FAST_HVX
#define RIPPLE_TANHF32_FAST_HVX

//===------------------------ ripple_tanh_f32_fast.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//   Internal header to access a custom fast tanh f32 vector implementation.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

#include "ripple_internal_utils_hvx.h"
#include "ripple_trunc_round_f32_f16.h"

alignas(sizeof(HVX_Vector)) static const
    float tanh_f32_fast_AB_poly_lookup[32] = {
        // As
        1.0f, 0.9999996276700475f, 0.9998979561346906f, 0.998229439379031f,
        -0.04580362186972309f, -0.11512074877868628f, -0.08488370228219644f,
        -0.04559148588439221f, -0.020414499856828305f, -0.007923773601525297f,
        -0.002668313051128246f, -0.00075767900528452f, -0.00016913584714146022f,
        -2.4897650411336838e-05f, -7.790480129997993e-07f, 0.0f,
        // Bs
        -0.33330053016105005f, -0.3332991322552843f, -0.33173792796442036f,
        -0.3217304395831384f, 0.005550799228128511f, 0.01009809399405559f,
        0.007863110322002043f, 0.004666952083348441f, 0.0023686426811136974f,
        0.001067520837507524f, 0.0004297203508572302f, 0.00015191818354703956f,
        4.5189672328718906e-05f, 1.0439931004926038e-05f,
        1.7855245659581867e-06f, 7.391710337298526e-07f};

alignas(sizeof(HVX_Vector)) static const
    float tanh_f32_fast_CD_poly_lookup[32] = {
        // Cs
        0.130685551185268f, 0.13237443491971135f, 0.12444561151756517f,
        0.10444823624054145f, -0.000207130079126705f, -0.0003065468013452241f,
        -0.0002514858128747771f, -0.0001648238253605834f,
        -9.48895054766587e-05f, -4.971205393896347e-05f,
        -2.391160068361889e-05f, -1.0448217644508026e-05f,
        -3.9972000451186495e-06f, -1.2070021049050535e-06f,
        -1.7225406201302106e-07f, 1.9912933627803447e-07f,
        // Ds
        0.0f, -0.04596393794679443f, -0.03266687346583197f,
        -0.019354250014162133f, 2.59408772764402e-06f, 3.3184521732766703e-06f,
        2.8663368882236666e-06f, 2.0830738295666196e-06f,
        1.3737422120551303e-06f, 8.508659379481443e-07f, 5.02978980497413e-07f,
        2.8549213751240477e-07f, 1.555287401774154e-07f, 8.086170334867437e-08f,
        3.963891203169335e-08f, 0.0f};

// About 2.5X faster than standard tanh_f32
// Piecewise polynomial function
// continuous function
// continuous first derivative
// monotonic
// RMS error 2.566614e-06
// Max error 1.755443e-05
RIPPLE_INTRIN_INLINE HVX_Vector ripple_tanh_f32_fast_hvx(HVX_Vector xval) {
  // extract signbit and take absolute value
  const HVX_Vector sign_bit = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x80000000));
  xval = Q6_V_vand_VV(xval, Q6_V_vsplat_R(0x7FFFFFFF));

  // cut to domain
  // 8 - epsilon
  const HVX_Vector float_cutoff = Q6_V_vsplat_R(0x40ffffff);
  xval = Q6_Vw_vmin_VwVw(xval, float_cutoff);

  // small route (polynomial from |x| itself)
  // big route, polynomial from |x| - 8
  HVX_Vector const8 = Q6_V_vsplat_R(0x41000000);
  HVX_Vector alt_xval = _ripple_internal_Vsf_vsub_VsfVsf(xval, const8);
  HVX_Vector x2 = Q6_Vqf32_vmpy_VsfVsf(alt_xval, alt_xval);

  // get index
  // 1.0/4.1 = 0.24390243902439027f
  const HVX_Vector inv_multiplier =
      Q6_Vqf32_vadd_VsfVsf(Q6_V_vsplat_R(0x3e79c190), Q6_V_vzero());

  // qf32
  HVX_Vector inds = Q6_Vqf32_vmpy_Vqf32Vqf32(x2, inv_multiplier);
  // sf -> w -> correct ind
  inds = Q6_Vw_vmax_VwVw(
      Q6_Vw_vsub_VwVw(Q6_V_vsplat_R(15),
                      _ripple_internal_w_from_sf(Q6_Vsf_equals_Vqf32(inds))),
      Q6_V_vzero());

  // now, choose small route or big route, and compute the consumables
  // small route: indices below flip_ind, polynomial from |x| itself
  // big route:   indices >= flip_ind,    polynomial from |x| - 8
  const HVX_Vector flip_ind = Q6_V_vsplat_R(4);
  HVX_VectorPred q_smallroute = Q6_Q_vcmp_gt_VwVw(flip_ind, inds);
  xval = Q6_Vqf32_vadd_VsfVsf(Q6_V_vmux_QVV(q_smallroute, xval, alt_xval),
                              Q6_V_vzero());
  // offset is 1 for the large values, 0 for the small ones.
  HVX_Vector offset = Q6_Vqf32_vadd_VsfVsf(
      Q6_V_vmux_QVV(q_smallroute, Q6_V_vzero(), Q6_V_vsplat_R(0x3f800000)),
      Q6_V_vzero());
  x2 = Q6_Vqf32_vmpy_Vqf32Vqf32(xval, xval); // needs recomputing

  // look up vals
  const HVX_Vector AB_table = *(HVX_Vector const *)tanh_f32_fast_AB_poly_lookup;
  const HVX_Vector CD_table = *(HVX_Vector const *)tanh_f32_fast_CD_poly_lookup;
  const HVX_Vector a = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, AB_table, 0), Q6_V_vzero());
  const HVX_Vector b = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, AB_table, 1), Q6_V_vzero());
  const HVX_Vector c = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, CD_table, 0), Q6_V_vzero());
  const HVX_Vector d = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, CD_table, 1), Q6_V_vzero());

  // result = offset + a*xval + b * xval^3 + c * xval^5 + d * xval^7
  // guaranteed to match value and first derivative at the boundaries of the
  // different polynomial domains use vals (polynomials)
  HVX_Vector result =
      Q6_Vqf32_vadd_Vqf32Vqf32(c, Q6_Vqf32_vmpy_Vqf32Vqf32(d, x2));
  result = Q6_Vqf32_vadd_Vqf32Vqf32(b, Q6_Vqf32_vmpy_Vqf32Vqf32(result, x2));
  result = Q6_Vqf32_vadd_Vqf32Vqf32(a, Q6_Vqf32_vmpy_Vqf32Vqf32(result, x2));
  result = Q6_Vqf32_vmpy_Vqf32Vqf32(result, xval);
  // add the constant offset:
  result = Q6_Vqf32_vadd_Vqf32Vqf32(result, offset);
  // convert to sf, add back sign bit, and return
  result = Q6_Vsf_equals_Vqf32(result);

  // add back the sign bit and return
  return Q6_V_vor_VV(sign_bit, result);
}

#endif
