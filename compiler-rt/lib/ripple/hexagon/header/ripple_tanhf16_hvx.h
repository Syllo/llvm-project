#ifndef RIPPLE_TANHF16_HVX
#define RIPPLE_TANHF16_HVX

//===--------------------------- ripple_tanhf16.h ---------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access a custom tanh f16 vector implementation.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_internal_utils_hvx.h"
#include <hexagon_types.h>
#include <stdbool.h>

// tanh table: map x2 -> tanh(x)
// where x2 = (1-x1)^2,  x1 = min( 1, abs(x)/(16/3))
//  so x2 in 0..1.0, in 16 cubic segments.
//  subtable  0 : 0,2,   ... 30    c3            (23 frac bits)
//  subtable  1 : 32,34, ... 62    c2            (20)
//  subtable  2 : 1,3,   ... 31    c1            (17)
//  subtable  3 : 33,35, ... 63    c0            (15)
//
// Ths is compensated for calculation of x2 by (~x1)^2 >> 16
// .. where x1,x2 are both u16 (0...1.0 with 16 fractional bits); e.g
// x1 = 0 => x2 = 0xfffe gives a zero result.
__attribute__((aligned(128))) static const int16_t tanh_16_cubic_lookup[64] = {
    -1125,  -58,    -742,   -117,   -1192,  -250,   -1725,  -475,
    -2424,  -837,   -3266,  -1392,  -4178,  -2214,  -5006,  -3385,
    -5405,  -4987,  -4952,  -7078,  -3158,  -9658,  262,    -12622,
    4941,   -15726, 9594,   -18595, 12789,  -20788, 13087,  -21927,
    -36,    32766,  -390,   32746,  -672,   32702,  -1115,  32613,
    -1760,  32453,  -2667,  32179,  -3894,  31735,  -5465,  31044,
    -7354,  30007,  -9406,  28509,  -11296, 26426,  -12509, 23647,
    -12443, 20101,  -10586, 15800,  -6953,  10858,  -2101,  5494,
};

// iS_TANH flag can be useful for other ops, like swish
//   -- if 'false', the function computes 0.5 + 0.5*tanh instead of tanh
RIPPLE_INTRIN_INLINE HVX_Vector ripple_tanhf16_hvx(HVX_Vector xval,
                                                   bool IS_TANH) {
  // first, remove the sign
  HVX_VectorPred qsign = Q6_Q_vcmp_gt_VhVh(Q6_V_vzero(), xval);
  if (!IS_TANH) {
    xval = _ripple_internal_Vhf_vmpy_VhfVhf(
        xval, Q6_Vh_vsplat_R(0x3800)); // input*0.5
  }
  HVX_Vector xabs = Q6_V_vand_VV(xval, Q6_Vh_vsplat_R(0x7fff));
  // is it >= 0.25 ?
  HVX_VectorPred q_bignum =
      Q6_Q_vcmp_gt_VuhVuh(xabs, Q6_Vh_vsplat_R(0x3400 - 1)); // >=0.25
  // >>1 again, first clip to <= 5.0
  xabs = Q6_Vuh_vmin_VuhVuh(xabs, Q6_Vh_vsplat_R(0x4500)); // clip to 5.0

  // 'small' path (for q_bignum false)
  //  tanh = x +  (-0.325*x)*(x*x)
  //  (note, this is more accurate than  x*(1 - 0.325*x*x), which gives
  //  rounding errors about twice as large).
  //
  // (1) find -0.325*x and x*x
  HVX_Vector tmp =
      Q6_Vqf16_vmpy_VhfVhf(xabs, Q6_Vh_vsplat_R(0xb533)); // -0.325*x
  HVX_Vector xabs2 = Q6_Vqf16_vmpy_VhfVhf(xabs, xabs);    // x*x
  // (2) find -0.325 * x^3
  tmp = Q6_Vqf16_vmpy_Vqf16Vqf16(tmp, xabs2);
  // (3) add xabs
  tmp = Q6_Vqf16_vadd_Vqf16Vhf(tmp, xabs);
  HVX_Vector small_result = Q6_Vhf_equals_Vqf16(tmp);

  // 'big' path - code is in range 0x3400..4500  (0.25 .. 5.0)
  // first, convert that to u16, multiplying by 4096 in the process.
  // (will be lossless)

  // NOTE: Q6_V_vand_QR(q_bignum, n) and Q6_V_vandor_VQR( v, q_bignum, n )
  // are used in a few places here as way of splatting; q_bignum is always
  // true in relevant lanes. HVX compiler tends to aggressively hoist
  // vsplat(invariant), to the point where it runs out of vector regs; this
  // approach avoids that since it can't be hoisted (and sometimes the 'or' is
  // useful too, as when inserting the hidden bit).

  HVX_Vector mant = Q6_V_vand_VV(xabs, Q6_Vh_vsplat_R(0x3FF));
  mant = Q6_V_vandor_VQR(mant, q_bignum, 0x04000400);
  HVX_Vector expo = Q6_Vuh_vlsr_VuhR(xabs, 10);                  // 13 ..17
  expo = Q6_Vh_vsub_VhVh(expo, Q6_Vh_vsplat_R(13));              // 0..4
  HVX_Vector x_u16 = Q6_Vh_vasl_VhVh(mant, expo);                //
  x_u16 = Q6_Vh_vadd_VhVh(Q6_Vh_vadd_VhVh(x_u16, x_u16), x_u16); //*3
  // we now have a value which is 4096*3*x, as integer;
  // for cases 0.25 <= x < 5, it is thus in  range 3072..61440.
  // This is the input to the 'fixed-point' tanh algo.
  //--------------------------------------------
  // invert it, square it, and keep the upper 16 bits only
  HVX_Vector x2 = Q6_V_vnot_V(x_u16);                        // invert
  HVX_VectorPair x1sq = Q6_Wuw_vmpy_VuhVuh(x2, x2);          // square that
  x2 = Q6_Vh_vshuffo_VhVh(Q6_V_hi_W(x1sq), Q6_V_lo_W(x1sq)); // >>16

  // extract the table select code: x2 >> 12
  HVX_Vector tablesel = Q6_Vuh_vlsr_VuhR(x2, 12);
  HVX_Vector table = *(HVX_Vector const *)tanh_16_cubic_lookup;
  // extract the 'residue' : 12 lsbs
  // get the coefficients from table
  HVX_Vector c3 = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR_nomatch(tablesel, table, 0));
  HVX_Vector c2 = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR_nomatch(tablesel, table, 1));
  HVX_Vector c1 = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR_nomatch(tablesel, table, 2));
  HVX_Vector c0 = Q6_V_lo_W(Q6_Wh_vlut16_VbVhR_nomatch(tablesel, table, 3));

  // poly
  HVX_Vector xres = Q6_V_vand_VV(x2, Q6_V_vsplat_R(0x0FFF0FFF));
  HVX_Vector resx2 = Q6_Vh_vadd_VhVh(xres, xres);
  HVX_Vector rtmp = Q6_Vh_vadd_VhVh_sat(Q6_Vh_vmpy_VhVh_s1_rnd_sat(c3, xres),
                                        c2); // 20 bit intermediate
  rtmp = Q6_Vh_vadd_VhVh_sat(Q6_Vh_vmpy_VhVh_s1_rnd_sat(rtmp, xres),
                             c1); // 17 bit intermediate
  HVX_Vector result = Q6_Vh_vadd_VhVh_sat(
      Q6_Vh_vmpy_VhVh_s1_rnd_sat(rtmp, resx2), c0); // 15 bit result
  // ------------------------------------
  // 'result' is tanh(x) with 15 fractional bits, and
  // is thus in range 8025.. 32767  (0.2449 ...1.0)
  // now we need to convert that value to fp16. Start by
  // adding a rounding bias, being 2,4, or 8, depending on whether
  //  result >= 4095,8192,16384.
  //
  HVX_Vector rbias =
      Q6_V_vand_QR(q_bignum, 0x00100010); // '16' in all relevant lanes
  rbias = Q6_Vh_vasr_VhVh(rbias, Q6_Vuh_vcl0_Vuh(result)); // 16 >> by 1,2 or 3,
  result = Q6_Vh_vadd_VhVh(result, rbias); // this can overflow to >=32768!
  // count leading zeros again
  HVX_Vector lz = Q6_Vuh_vcl0_Vuh(result);        // 0,1,2 or 3
  HVX_Vector m_tmp = Q6_Vh_vasl_VhVh(result, lz); // normalize.
  m_tmp = Q6_Vuh_vlsr_VuhR(m_tmp, 5);             // and >> 5 to position it.
  // bit 9..0 are the mantissa; bit 10 is the hidden bit which we will remove.
  // now, we need to insert (15-lz)<<10, which is one of
  // 0x3C00,0x3800,0x3400,0x3000 Or: or in 0x3c00 (which overwrites the hidden
  // bit), and then subtract lz <<10
  m_tmp = Q6_V_vandor_VQR(m_tmp, q_bignum, 0x3C003C00);
  HVX_Vector big_result = Q6_Vh_vsub_VhVh(m_tmp, Q6_Vh_vasl_VhR(lz, 10));
  // and that's it - but need to select big or little

  HVX_Vector final_tanh = Q6_V_vmux_QVV(q_bignum, big_result, small_result);
  // and put sign in
  final_tanh = Q6_V_vandor_VQR(final_tanh, qsign, 0x80008000);

  if (!IS_TANH) {
    // 0.5*tanh + 0.5
    final_tanh = Q6_Vhf_equals_Vqf16(Q6_Vqf16_vmpy_Vqf16Vhf(
        Q6_Vqf16_vadd_VhfVhf(final_tanh, Q6_Vh_vsplat_R(0x3C00)),
        Q6_Vh_vsplat_R(0x3800)));
  }
  // bound to 1
  final_tanh = Q6_Vhf_vmin_VhfVhf(final_tanh, Q6_Vh_vsplat_R(0x3C00));
  return final_tanh;
}
#endif
