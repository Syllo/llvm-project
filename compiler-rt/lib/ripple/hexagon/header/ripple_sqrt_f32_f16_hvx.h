#ifndef RIPPLE_SQRT_F32_F16_HVX
#define RIPPLE_SQRT_F32_F16_HVX

//===--------------------- ripple_sqrt_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
// Internal header to access sqrt and rsqrt f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include <hexagon_types.h>
#include <stdbool.h>

// safe = handles subnormals (roughly triples runtime)
// full = 2 runs of Newton's method instead of 1

RIPPLE_INTRIN_INLINE HVX_Vector ripple_rsqrt_f32(HVX_Vector val, bool safe,
                                                 bool full) {
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7F800000);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1 = Q6_V_vsplat_R(1);
  const HVX_Vector const2pow24 = Q6_V_vsplat_R(0x4b800000);
  const HVX_Vector const2pow12 = Q6_V_vsplat_R(0x45800000);

  const HVX_Vector constNan = Q6_V_vsplat_R(0x7fc00000);

  const HVX_Vector constMagic = Q6_V_vsplat_R(0x5f3759df);
  const HVX_Vector const_3div2 = Q6_V_vsplat_R(0x3fc00000);
  const HVX_Vector const_qf32_3div2 = Q6_Vqf32_vadd_VsfVsf(const_3div2, const0);
  const HVX_Vector const1div2 = Q6_V_vsplat_R(0x3f000000);

  HVX_VectorPred is_subnormal;
  // if val == 0 or sign bit set, val looks like negative word
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VwVw(const1, val);

  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VwVw(const0, Q6_V_vand_VV(val, exp_extract));
    HVX_Vector alt_val = _ripple_internal_Vsf_vmpy_VsfVsf(val, const2pow24);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }

  HVX_Vector x2 = Q6_Vqf32_vmpy_VsfVsf(val, const1div2); // qf32

  // main pointer-hacking operation
  val = Q6_Vw_vsub_VwVw(constMagic, Q6_Vw_vasr_VwR(val, 1));

  // qf32 polynomial approx. stuff
  HVX_Vector qval = Q6_Vqf32_vadd_VsfVsf(val, const0);
  HVX_Vector temp = Q6_Vqf32_vmpy_Vqf32Vqf32(qval, qval);
  temp = Q6_Vqf32_vmpy_Vqf32Vqf32(x2, temp);
  temp = Q6_Vqf32_vsub_Vqf32Vqf32(const_qf32_3div2, temp);
  temp = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, qval);

  if (full) {
    qval = Q6_Vqf32_vadd_Vqf32Vsf(temp, const0);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(qval, qval);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(x2, temp);
    temp = Q6_Vqf32_vsub_Vqf32Vqf32(const_qf32_3div2, temp);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, qval);
  }

  val = Q6_Vsf_equals_Vqf32(temp);
  if (safe) {
    HVX_Vector alt_val = _ripple_internal_Vsf_vmpy_VsfVsf(val, const2pow12);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }
  val = Q6_V_vmux_QVV(is_illegal, constNan, val);
  return val;
}

// just like rsqrt, but we multiply by the original value at the end.
RIPPLE_INTRIN_INLINE HVX_Vector ripple_sqrt_f32(HVX_Vector x, bool safe,
                                                bool full) {
  const HVX_Vector exp_extract = Q6_V_vsplat_R(0x7F800000);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const2pow24 = Q6_V_vsplat_R(0x4b800000);
  const HVX_Vector const2powNeg12 = Q6_V_vsplat_R(0x39800000);

  const HVX_Vector constNan = Q6_V_vsplat_R(0x7fc00000);

  const HVX_Vector constMagic = Q6_V_vsplat_R(0x5f3759df);
  const HVX_Vector const_3div2 = Q6_V_vsplat_R(0x3fc00000);
  const HVX_Vector const_qf32_3div2 = Q6_Vqf32_vadd_VsfVsf(const_3div2, const0);
  const HVX_Vector const1div2 = Q6_V_vsplat_R(0x3f000000);

  HVX_VectorPred is_subnormal;
  // if sign bit set, val looks like negative word
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VwVw(const0, x);

  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VwVw(const0, Q6_V_vand_VV(x, exp_extract));
    HVX_Vector alt_x = _ripple_internal_Vsf_vmpy_VsfVsf(x, const2pow24);
    x = Q6_V_vmux_QVV(is_subnormal, alt_x, x);
  }

  HVX_Vector x2 = Q6_Vqf32_vmpy_VsfVsf(x, const1div2); // qf32

  // main pointer-hacking operation
  HVX_Vector val = Q6_Vw_vsub_VwVw(constMagic, Q6_Vw_vasr_VwR(x, 1));

  // qf32 polynomial approx. stuff
  HVX_Vector qval = Q6_Vqf32_vadd_VsfVsf(val, const0);
  HVX_Vector temp = Q6_Vqf32_vmpy_Vqf32Vqf32(qval, qval);
  temp = Q6_Vqf32_vmpy_Vqf32Vqf32(x2, temp);
  temp = Q6_Vqf32_vsub_Vqf32Vqf32(const_qf32_3div2, temp);
  temp = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, qval);

  if (full) {
    qval = Q6_Vqf32_vadd_Vqf32Vsf(temp, const0);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(qval, qval);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(x2, temp);
    temp = Q6_Vqf32_vsub_Vqf32Vqf32(const_qf32_3div2, temp);
    temp = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, qval);
  }

  // biggest difference between rsqrt and sqrt is this line
  val = Q6_Vqf32_vmpy_Vqf32Vqf32(temp, Q6_Vqf32_vadd_VsfVsf(x, const0));
  val = Q6_Vsf_equals_Vqf32(val);

  // +40% time, but what can you do? sqrt(0) cannot be a Nan
  if (full) {
    HVX_VectorPred isZero = Q6_Q_vcmp_eq_VwVw(x, const0);
    val = Q6_V_vmux_QVV(isZero, const0, val);
  }

  if (safe) {
    HVX_Vector alt_val = _ripple_internal_Vsf_vmpy_VsfVsf(val, const2powNeg12);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }
  val = Q6_V_vmux_QVV(is_illegal, constNan, val);

  return val;
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_rsqrt_f16_promote(HVX_Vector x) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  HVX_VectorPair qfx = Q6_Wqf32_vmpy_VhfVhf(x, const1);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_rsqrt_f32(Q6_Vsf_equals_Vqf32(Q6_V_hi_W(qfx)), false, false),
      Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_rsqrt_f32(Q6_Vsf_equals_Vqf32(Q6_V_lo_W(qfx)), false, false),
      Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vdeal_VVR(hiRes, loRes, 0);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_rsqrt_f16(HVX_Vector val, bool safe,
                                                 bool full) {
  const HVX_Vector exp_extract = Q6_Vh_vsplat_R(0x7c00);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const1 = Q6_Vh_vsplat_R(1);
  const HVX_Vector const2pow10 = Q6_Vh_vsplat_R(0x6400);
  const HVX_Vector const2pow5 = Q6_Vh_vsplat_R(0x5000);

  const HVX_Vector constNan = Q6_Vh_vsplat_R(0x7e00);

  const HVX_Vector constMagic = Q6_Vh_vsplat_R(0x59ba);
  const HVX_Vector const_3div2 = Q6_Vh_vsplat_R(0x3e00);
  const HVX_Vector const_qf16_3div2 = Q6_Vqf16_vadd_VhfVhf(const_3div2, const0);
  const HVX_Vector const1div2 = Q6_Vh_vsplat_R(0x3800);

  HVX_VectorPred is_subnormal;
  // if val == 0 or sign bit set, val looks like negative word
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VhVh(const1, val);

  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VhVh(const0, Q6_V_vand_VV(val, exp_extract));
    HVX_Vector alt_val = _ripple_internal_Vhf_vmpy_VhfVhf(val, const2pow10);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }

  HVX_Vector x2 = Q6_Vqf16_vmpy_VhfVhf(val, const1div2); // qf16

  // main pointer-hacking operation
  val = Q6_Vh_vsub_VhVh(constMagic, Q6_Vh_vasr_VhR(val, 1));

  // qf16 polynomial approx. stuff
  HVX_Vector qval = Q6_Vqf16_vadd_VhfVhf(val, const0);
  HVX_Vector temp = Q6_Vqf16_vmpy_Vqf16Vqf16(qval, qval);
  temp = Q6_Vqf16_vmpy_Vqf16Vqf16(x2, temp);
  temp = Q6_Vqf16_vsub_Vqf16Vqf16(const_qf16_3div2, temp);
  temp = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, qval);

  if (full) {
    qval = Q6_Vqf16_vadd_Vqf16Vhf(temp, const0);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(qval, qval);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(x2, temp);
    temp = Q6_Vqf16_vsub_Vqf16Vqf16(const_qf16_3div2, temp);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, qval);
  }

  val = Q6_Vhf_equals_Vqf16(temp);

  if (safe) {
    HVX_Vector alt_val = _ripple_internal_Vhf_vmpy_VhfVhf(val, const2pow5);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }
  val = Q6_V_vmux_QVV(is_illegal, constNan, val);

  return val;
}

//===------------------------------------------------------------------------===
//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector ripple_sqrt_f16_promote(HVX_Vector x) {
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  HVX_VectorPair qfx = Q6_Wqf32_vmpy_VhfVhf(x, const1);

  HVX_Vector hiRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_sqrt_f32(Q6_Vsf_equals_Vqf32(Q6_V_hi_W(qfx)), false, false),
      Q6_V_vzero());
  HVX_Vector loRes = Q6_Vqf32_vadd_VsfVsf(
      ripple_sqrt_f32(Q6_Vsf_equals_Vqf32(Q6_V_lo_W(qfx)), false, false),
      Q6_V_vzero());

  HVX_VectorPair qfres = Q6_W_vdeal_VVR(hiRes, loRes, 0);
  HVX_Vector res = Q6_Vhf_equals_Wqf32(qfres);
  return res;
}

// just like rsqrt, but we multiply by the input at the end
RIPPLE_INTRIN_INLINE HVX_Vector ripple_sqrt_f16(HVX_Vector x, bool safe,
                                                bool full) {
  const HVX_Vector exp_extract = Q6_Vh_vsplat_R(0x7c00);

  const HVX_Vector const0 = Q6_V_vzero();
  const HVX_Vector const2pow10 = Q6_Vh_vsplat_R(0x6400);
  const HVX_Vector const2powNeg5 = Q6_Vh_vsplat_R(0x2800);

  const HVX_Vector constNan = Q6_Vh_vsplat_R(0x7e00);

  const HVX_Vector constMagic = Q6_Vh_vsplat_R(0x59ba);
  const HVX_Vector const_3div2 = Q6_Vh_vsplat_R(0x3e00);
  const HVX_Vector const_qf16_3div2 = Q6_Vqf16_vadd_VhfVhf(const_3div2, const0);
  const HVX_Vector const1div2 = Q6_Vh_vsplat_R(0x3800);

  HVX_VectorPred is_subnormal;
  // if sign bit set, val looks like negative word
  const HVX_VectorPred is_illegal = Q6_Q_vcmp_gt_VhVh(const0, x);

  if (safe) {
    is_subnormal = Q6_Q_vcmp_eq_VhVh(const0, Q6_V_vand_VV(x, exp_extract));
    HVX_Vector alt_x = _ripple_internal_Vhf_vmpy_VhfVhf(x, const2pow10);
    x = Q6_V_vmux_QVV(is_subnormal, alt_x, x);
  }

  HVX_Vector x2 = Q6_Vqf16_vmpy_VhfVhf(x, const1div2); // qf16

  // main pointer-hacking operation
  HVX_Vector val = Q6_Vh_vsub_VhVh(constMagic, Q6_Vh_vasr_VhR(x, 1));

  // qf16 polynomial approx. stuff
  HVX_Vector qval = Q6_Vqf16_vadd_VhfVhf(val, const0);
  HVX_Vector temp = Q6_Vqf16_vmpy_Vqf16Vqf16(qval, qval);
  temp = Q6_Vqf16_vmpy_Vqf16Vqf16(x2, temp);
  temp = Q6_Vqf16_vsub_Vqf16Vqf16(const_qf16_3div2, temp);
  temp = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, qval);

  if (full) {
    qval = Q6_Vqf16_vadd_Vqf16Vhf(temp, const0);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(qval, qval);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(x2, temp);
    temp = Q6_Vqf16_vsub_Vqf16Vqf16(const_qf16_3div2, temp);
    temp = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, qval);
  }

  // biggest change with rsqrt is this line
  val = Q6_Vqf16_vmpy_Vqf16Vqf16(temp, Q6_Vqf16_vadd_VhfVhf(x, const0));
  val = Q6_Vhf_equals_Vqf16(val);

  // +40% time, but what can you do? sqrt(0) cannot be a Nan
  if (full) {
    HVX_VectorPred isZero = Q6_Q_vcmp_eq_VhVh(x, const0);
    val = Q6_V_vmux_QVV(isZero, const0, val);
  }

  if (safe) {
    HVX_Vector alt_val = _ripple_internal_Vhf_vmpy_VhfVhf(val, const2powNeg5);
    val = Q6_V_vmux_QVV(is_subnormal, alt_val, val);
  }
  val = Q6_V_vmux_QVV(is_illegal, constNan, val);

  return val;
}

#endif
