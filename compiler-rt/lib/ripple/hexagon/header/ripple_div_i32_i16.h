#ifndef RIPPLE_DIV_I32_I16_HVX
#define RIPPLE_DIV_I32_I16_HVX

//===---------------------- ripple_div_i32_i16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access a exp i32 and i16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>

//===------------------------------------------------------------------------===
// This header defines several internal implementations of vector integer div
//===------------------------------------------------------------------------===

struct ripple_int_div_results {
  HVX_Vector div;
  HVX_Vector rem;
};

// unsigned uh division; produces result and remainder
RIPPLE_INTRIN_INLINE struct ripple_int_div_results
ripple_integer_div_rem_uh(HVX_Vector n, HVX_Vector d) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x0001);
  const HVX_Vector const2 = Q6_Vh_vsplat_R(0x0002);
  const HVX_Vector const4 = Q6_Vh_vsplat_R(0x0004);
  const HVX_Vector const8 = Q6_Vh_vsplat_R(0x0008);

  const HVX_Vector constF = Q6_Vh_vsplat_R(0x000F);

  const HVX_Vector test_need2 = Q6_Vh_vsplat_R(0x8000);
  const HVX_VectorPred need2 = Q6_Q_vcmp_gt_VuhVuh(test_need2, d);
  const HVX_Vector d2 = Q6_Vh_vasl_VhR(d, 1);

  const HVX_Vector test_need4 = Q6_Vh_vsplat_R(0x4000);
  const HVX_VectorPred need4 = Q6_Q_vcmp_gt_VuhVuh(test_need4, d);
  const HVX_Vector d4 = Q6_Vh_vasl_VhR(d, 2);

  const HVX_Vector test_need8 = Q6_Vh_vsplat_R(0x2000);
  const HVX_VectorPred need8 = Q6_Q_vcmp_gt_VuhVuh(test_need8, d);
  const HVX_Vector d8 = Q6_Vh_vasl_VhR(d, 3);

  HVX_Vector rem = Q6_V_vzero();
  HVX_Vector div = Q6_V_vzero();

  for (int i = 3; i > -1; i--) {
    div = Q6_Vh_vasl_VhR(div, 4);
    rem = Q6_Vh_vasl_VhR(rem, 4);
    // ith digit of n becomes 0th digit of rem
    rem = Q6_V_vor_VV(rem, Q6_V_vand_VV(Q6_Vuh_vlsr_VuhR(n, 4 * i), constF));
    // compute alternative values and whether or not we need them
    HVX_Vector temp_rem = Q6_Vuh_vsub_VuhVuh_sat(rem, d8);
    HVX_Vector temp_div = Q6_V_vor_VV(div, const8);
    // have to do it this way for the special case n==constMax
    HVX_VectorPred change = Q6_Q_and_QQn(need8, Q6_Q_vcmp_gt_VuhVuh(d8, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuh_vsub_VuhVuh_sat(rem, d4);
    temp_div = Q6_V_vor_VV(div, const4);
    change = Q6_Q_and_QQn(need4, Q6_Q_vcmp_gt_VuhVuh(d4, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuh_vsub_VuhVuh_sat(rem, d2);
    temp_div = Q6_V_vor_VV(div, const2);
    change = Q6_Q_and_QQn(need2, Q6_Q_vcmp_gt_VuhVuh(d2, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuh_vsub_VuhVuh_sat(rem, d);
    temp_div = Q6_V_vor_VV(div, const1);
    change = Q6_Q_not_Q(Q6_Q_vcmp_gt_VuhVuh(d, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
  }
  struct ripple_int_div_results result = {div, rem};
  return result;
}

// signed h division
// +n, +d : keep the uh result
// flipped signs: flip the sign of div
// -n: make the modulus negative
RIPPLE_INTRIN_INLINE struct ripple_int_div_results
ripple_integer_div_rem_h(HVX_Vector n, HVX_Vector d) {
  const HVX_Vector const0 = Q6_V_vzero();

  HVX_Vector nabs = Q6_Vh_vabs_Vh(n);
  HVX_Vector dabs = Q6_Vh_vabs_Vh(d);
  struct ripple_int_div_results abs_result =
      ripple_integer_div_rem_uh(nabs, dabs);

  HVX_Vector negRem = Q6_Vh_vsub_VhVh(const0, abs_result.rem);
  HVX_VectorPred negN = Q6_Q_vcmp_gt_VhVh(const0, n);
  HVX_Vector rem = Q6_V_vmux_QVV(negN, negRem, abs_result.rem);

  HVX_VectorPred negD = Q6_Q_vcmp_gt_VhVh(const0, d);
  HVX_VectorPred oppSign = Q6_Q_xor_QQ(negN, negD);

  HVX_Vector negDiv = Q6_Vh_vsub_VhVh(const0, abs_result.div);
  HVX_Vector div = Q6_V_vmux_QVV(oppSign, negDiv, abs_result.div);

  struct ripple_int_div_results result = {div, rem};
  return result;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_div_uh(HVX_Vector n,
                                                      HVX_Vector d) {
  return ripple_integer_div_rem_uh(n, d).div;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_rem_uh(HVX_Vector n,
                                                      HVX_Vector d) {
  return ripple_integer_div_rem_uh(n, d).rem;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_div_h(HVX_Vector n,
                                                     HVX_Vector d) {
  return ripple_integer_div_rem_h(n, d).div;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_rem_h(HVX_Vector n,
                                                     HVX_Vector d) {
  return ripple_integer_div_rem_h(n, d).rem;
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

// unsigned uw division; produces result and remainder
RIPPLE_INTRIN_INLINE struct ripple_int_div_results
ripple_integer_div_rem_uw(HVX_Vector n, HVX_Vector d) {
  const HVX_Vector const1 = Q6_V_vsplat_R(0x00000001);
  const HVX_Vector const2 = Q6_V_vsplat_R(0x00000002);
  const HVX_Vector const4 = Q6_V_vsplat_R(0x00000004);
  const HVX_Vector const8 = Q6_V_vsplat_R(0x00000008);

  const HVX_Vector constF = Q6_V_vsplat_R(0x0000000F);

  const HVX_Vector test_need2 = Q6_V_vsplat_R(0x80000000);
  const HVX_VectorPred need2 = Q6_Q_vcmp_gt_VuwVuw(test_need2, d);
  const HVX_Vector d2 = Q6_Vw_vasl_VwR(d, 1);

  const HVX_Vector test_need4 = Q6_V_vsplat_R(0x40000000);
  const HVX_VectorPred need4 = Q6_Q_vcmp_gt_VuwVuw(test_need4, d);
  const HVX_Vector d4 = Q6_Vw_vasl_VwR(d, 2);

  const HVX_Vector test_need8 = Q6_V_vsplat_R(0x20000000);
  const HVX_VectorPred need8 = Q6_Q_vcmp_gt_VuwVuw(test_need8, d);
  const HVX_Vector d8 = Q6_Vw_vasl_VwR(d, 3);

  HVX_Vector rem = Q6_V_vzero();
  HVX_Vector div = Q6_V_vzero();

  for (int i = 7; i > -1; i--) {
    div = Q6_Vw_vasl_VwR(div, 4);
    rem = Q6_Vw_vasl_VwR(rem, 4);
    // ith digit of n becomes 0th digit of rem
    rem = Q6_V_vor_VV(rem, Q6_V_vand_VV(Q6_Vuw_vlsr_VuwR(n, 4 * i), constF));
    // compute alternative values and whether or not we need them
    HVX_Vector temp_rem = Q6_Vuw_vsub_VuwVuw_sat(rem, d8);
    HVX_Vector temp_div = Q6_V_vor_VV(div, const8);
    // have to do it this way for the special case n==constMax
    HVX_VectorPred change = Q6_Q_and_QQn(need8, Q6_Q_vcmp_gt_VuwVuw(d8, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuw_vsub_VuwVuw_sat(rem, d4);
    temp_div = Q6_V_vor_VV(div, const4);
    change = Q6_Q_and_QQn(need4, Q6_Q_vcmp_gt_VuwVuw(d4, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuw_vsub_VuwVuw_sat(rem, d2);
    temp_div = Q6_V_vor_VV(div, const2);
    change = Q6_Q_and_QQn(need2, Q6_Q_vcmp_gt_VuwVuw(d2, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
    temp_rem = Q6_Vuw_vsub_VuwVuw_sat(rem, d);
    temp_div = Q6_V_vor_VV(div, const1);
    change = Q6_Q_not_Q(Q6_Q_vcmp_gt_VuwVuw(d, rem));
    // mux temp values into real values
    rem = Q6_V_vmux_QVV(change, temp_rem, rem);
    div = Q6_V_vmux_QVV(change, temp_div, div);
    ///////////////////////////////////////
  }
  struct ripple_int_div_results result = {div, rem};
  return result;
}

// signed w division
// +n, +d : keep the uw result
// flipped signs: flip the sign of div
// -n: make the modulus negative
RIPPLE_INTRIN_INLINE struct ripple_int_div_results
ripple_integer_div_rem_w(HVX_Vector n, HVX_Vector d) {
  const HVX_Vector const0 = Q6_V_vzero();

  HVX_Vector nabs = Q6_Vw_vabs_Vw(n);
  HVX_Vector dabs = Q6_Vw_vabs_Vw(d);
  struct ripple_int_div_results abs_result =
      ripple_integer_div_rem_uw(nabs, dabs);

  HVX_Vector negRem = Q6_Vw_vsub_VwVw(const0, abs_result.rem);
  HVX_VectorPred negN = Q6_Q_vcmp_gt_VwVw(const0, n);
  HVX_Vector rem = Q6_V_vmux_QVV(negN, negRem, abs_result.rem);

  HVX_VectorPred negD = Q6_Q_vcmp_gt_VwVw(const0, d);
  HVX_VectorPred oppSign = Q6_Q_xor_QQ(negN, negD);

  HVX_Vector negDiv = Q6_Vw_vsub_VwVw(const0, abs_result.div);
  HVX_Vector div = Q6_V_vmux_QVV(oppSign, negDiv, abs_result.div);

  struct ripple_int_div_results result = {div, rem};
  return result;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_div_uw(HVX_Vector n,
                                                      HVX_Vector d) {
  return ripple_integer_div_rem_uw(n, d).div;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_rem_uw(HVX_Vector n,
                                                      HVX_Vector d) {
  return ripple_integer_div_rem_uw(n, d).rem;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_div_w(HVX_Vector n,
                                                     HVX_Vector d) {
  return ripple_integer_div_rem_w(n, d).div;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_integer_rem_w(HVX_Vector n,
                                                     HVX_Vector d) {
  return ripple_integer_div_rem_w(n, d).rem;
}
#endif
