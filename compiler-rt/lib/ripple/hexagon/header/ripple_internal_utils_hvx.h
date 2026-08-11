#ifndef RIPPLE_INTERNAL_UTILS
#define RIPPLE_INTERNAL_UTILS

//===--------------------- ripple_internal_utils_hvx.h ----------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//             Some useful functions that make writing code faster.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <stdbool.h>

// allows lookup for 16 w's from a NATURAL lookup table (16 floats in a row)
// They are fully stored in the bottom half-byte
// R is either 0 (for bottom half) or 1 (for top half); other values produce a
// '0' in output
//
// With safe=false, callers must provide valid lookup indices and R.
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_Vw_vlut16_VwVwR(
    HVX_Vector inds, HVX_Vector table, unsigned R, bool safe) {
  if (safe) {
    R = R & 0x1;
    inds = Q6_V_vand_VV(inds, Q6_V_vsplat_R(0xF));
  }
  inds = Q6_V_vor_VV(inds, Q6_V_vsplat_R(R << 4));
  HVX_Vector lo_vals = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR(inds, table, R));
  // filter out junk in upper half-words:
  lo_vals = Q6_V_vand_VV(lo_vals, Q6_V_vsplat_R(0x0000ffff));

  HVX_Vector shifted2 = Q6_V_vsplat_R(2 << 4);
  inds = Q6_V_vor_VV(inds, shifted2);

  // sign to use the odd half of available indeces
  HVX_Vector hi_vals = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR(inds, table, R | 2));
  hi_vals = Q6_Vw_vasl_VwR(hi_vals, 16);
  return Q6_V_vor_VV(hi_vals, lo_vals);
}

// This function consumes input values, indices (exotic xval -> index functions
// are user responsibility) and the AB / CD lookup tables. It spits out degree-7
// polynomials ( ax + bx^3 + cx^5 + dx^7 ), where a, b, c, and d are looked up
// using the same "index" for each value. That is, treating the input vectors as
// an array, this funciton computes for all i output[i] = (a[inds[i]] * x[i]) +
// (b[inds[i]] * x[i]^3) + (c[inds[i]] * x[i]^5) + (d[inds[i]] * x[i] ^ 7) Any
// pre/post-processing, along with ind computation, is the responsibility of the
// calling function.
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_piecewise_poly_engine(
    HVX_Vector xval, const HVX_Vector inds, const HVX_Vector AB_table,
    const HVX_Vector CD_table) {
  xval = Q6_Vqf32_vadd_VsfVsf(xval, Q6_V_vzero());
  HVX_Vector x2 = Q6_Vqf32_vmpy_Vqf32Vqf32(xval, xval);

  const HVX_Vector a = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, AB_table, 0, false), Q6_V_vzero());
  const HVX_Vector b = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, AB_table, 1, false), Q6_V_vzero());
  const HVX_Vector c = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, CD_table, 0, false), Q6_V_vzero());
  const HVX_Vector d = Q6_Vqf32_vadd_VsfVsf(
      _ripple_internal_Vw_vlut16_VwVwR(inds, CD_table, 1, false), Q6_V_vzero());

  // use vals (polynomials)
  HVX_Vector result =
      Q6_Vqf32_vadd_Vqf32Vqf32(c, Q6_Vqf32_vmpy_Vqf32Vqf32(d, x2));
  result = Q6_Vqf32_vadd_Vqf32Vqf32(b, Q6_Vqf32_vmpy_Vqf32Vqf32(result, x2));
  result = Q6_Vqf32_vadd_Vqf32Vqf32(a, Q6_Vqf32_vmpy_Vqf32Vqf32(result, x2));
  result = Q6_Vqf32_vmpy_Vqf32Vqf32(result, xval);

  // convert to sf
  result = Q6_Vsf_equals_Vqf32(result);
  return result;
}

//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_VectorPair
_ripple_internal_Wsf_vcvt_Vhf(HVX_Vector inp) {
#ifdef __HVX_IEEE_FP__
  return Q6_Wsf_vcvt_Vhf(inp);
#else
  const HVX_Vector const1 = Q6_Vh_vsplat_R(0x3c00);
  HVX_VectorPair qf_inp = Q6_Wqf32_vmpy_VhfVhf(inp, const1);
  HVX_Vector sf_hi = Q6_Vsf_equals_Vqf32(Q6_V_hi_W(qf_inp));
  HVX_Vector sf_lo = Q6_Vsf_equals_Vqf32(Q6_V_lo_W(qf_inp));
  return Q6_W_vcombine_VV(sf_hi, sf_lo);
#endif
}

//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vsf_vadd_VsfVsf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vsf_vadd_VsfVsf(inp1, inp2);
#else
  return Q6_Vsf_equals_Vqf32(Q6_Vqf32_vadd_VsfVsf(inp1, inp2));
#endif
}

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vsf_vsub_VsfVsf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vsf_vsub_VsfVsf(inp1, inp2);
#else
  return Q6_Vsf_equals_Vqf32(Q6_Vqf32_vsub_VsfVsf(inp1, inp2));
#endif
}

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vsf_vmpy_VsfVsf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vsf_vmpy_VsfVsf(inp1, inp2);
#else
  return Q6_Vsf_equals_Vqf32(Q6_Vqf32_vmpy_VsfVsf(inp1, inp2));
#endif
}

//===------------------------------------------------------------------------===

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vhf_vadd_VhfVhf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vhf_vadd_VhfVhf(inp1, inp2);
#else
  return Q6_Vhf_equals_Vqf16(Q6_Vqf16_vadd_VhfVhf(inp1, inp2));
#endif
}

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vhf_vsub_VhfVhf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vhf_vsub_VhfVhf(inp1, inp2);
#else
  return Q6_Vhf_equals_Vqf16(Q6_Vqf16_vsub_VhfVhf(inp1, inp2));
#endif
}

RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_Vhf_vmpy_VhfVhf(HVX_Vector inp1, HVX_Vector inp2) {
#ifdef __HVX_IEEE_FP__
  return Q6_Vhf_vmpy_VhfVhf(inp1, inp2);
#else
  return Q6_Vhf_equals_Vqf16(Q6_Vqf16_vmpy_VhfVhf(inp1, inp2));
#endif
}

#endif
