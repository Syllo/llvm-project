#ifndef RIPPLE_TRIG_F16
#define RIPPLE_TRIG_F16

//===-------------------------- ripple_trig_f16.h ---------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//  Internal header to access custom sin, cos, tan f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>

#include "ripple_div_f32_f16_hvx_fast.h"
#include "ripple_trunc_round_f32_f16.h"

// remainder in [0, 1) after multiplying by the scales
RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_find_remainder_sf(HVX_Vector input, const HVX_Vector scales) {
  HVX_Vector product = _ripple_internal_Vsf_vmpy_VsfVsf(input, scales);
  HVX_Vector noise = ripple_floor_f32(product);
  return _ripple_internal_Vsf_vsub_VsfVsf(product, noise);
}

// uses sf multiplication and rounding to find the remainder after
// dividing by 2pi.
// RETURNS QUANTIZED VALUE
// does not return an fp16, returns u16, where 0=0 and 0x10000 = 1.0
RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_quantized_remainder_hf(HVX_Vector input, const HVX_Vector scale) {
  const HVX_Vector one_hf = Q6_Vh_vsplat_R(0x3c00);
  // float(MAX_UINT16_T + 1)
  const HVX_Vector max_val =
      Q6_V_vsplat_R(0x47800000); // float(MAX_UINT16_T + 1)
  // max possible quantization; needed to handle overflow cases
  const HVX_Vector max_quantized_val = Q6_V_vsplat_R(0xffff);

  HVX_VectorPair modified = Q6_Wqf32_vmpy_VhfVhf(input, one_hf);

  // even indices are here
  HVX_Vector lo_res_sf = _ripple_find_remainder_sf(
      Q6_Vsf_equals_Vqf32(Q6_V_lo_W(modified)), scale);
  // odd indices are here
  HVX_Vector hi_res_sf = _ripple_find_remainder_sf(
      Q6_Vsf_equals_Vqf32(Q6_V_hi_W(modified)), scale);

  // now that the results are all >=0 and < 1, we can quantize them:
  HVX_Vector quant_lo_sf = _ripple_internal_Vsf_vmpy_VsfVsf(lo_res_sf, max_val);
  HVX_Vector quant_hi_sf = _ripple_internal_Vsf_vmpy_VsfVsf(hi_res_sf, max_val);

  HVX_Vector quant_lo_w = _ripple_internal_w_from_sf(quant_lo_sf);
  HVX_Vector quant_hi_w = _ripple_internal_w_from_sf(quant_hi_sf);

  // handle possible overflow cases, like NaN and inf
  quant_lo_w = Q6_Vw_vmin_VwVw(Q6_Vw_vabs_Vw(quant_lo_w), max_quantized_val);
  quant_hi_w = Q6_Vw_vmin_VwVw(Q6_Vw_vabs_Vw(quant_hi_w), max_quantized_val);

  // move odd vals to their spots
  quant_hi_w = Q6_Vw_vasl_VwR(quant_hi_w, 16);

  return Q6_V_vor_VV(quant_lo_w, quant_hi_w);
}

RIPPLE_INTRIN_INLINE HVX_Vector _ripple_requant_u14_to_hf(HVX_Vector input) {
  HVX_VectorPair input_w = Q6_Wuw_vmpy_VuhRuh(input, 0x10001);

  HVX_Vector inp_lo_w = Q6_V_lo_W(input_w);
  HVX_Vector inp_hi_w = Q6_V_hi_W(input_w);

  HVX_Vector inp_lo_sf = _ripple_internal_sf_from_w(inp_lo_w);
  HVX_Vector inp_hi_sf = _ripple_internal_sf_from_w(inp_hi_w);

  // 2^-14
  const HVX_Vector inv_scale = Q6_V_vsplat_R(0x38800000);
  HVX_Vector inp_lo_qf = Q6_Vqf32_vmpy_VsfVsf(inp_lo_sf, inv_scale);
  HVX_Vector inp_hi_qf = Q6_Vqf32_vmpy_VsfVsf(inp_hi_sf, inv_scale);

  HVX_VectorPair output_qf = Q6_W_vcombine_VV(inp_hi_qf, inp_lo_qf);
  return Q6_Vhf_equals_Wqf32(output_qf);
}

// ====================================================
// FUNCTION: _ripple_cospiby2x_approx     y(0) = 1,  y(0.5) = 0.7071, y(1)
// = 6.123e-17 Order:3; continuity: True; Ends forced: True Mode: unsigned;
// Result fractional bits: 14 Peak Error: 1.1706e-04  Rms Error: 2.6749e-05 Mean
// Error: 8.1020e-06
//      32769      24  -40985    1950
//      32634    1526  -46801    5902
//      31904    5846  -55430    8809
//      30619   11029  -62432   10394
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_cospiby2x_approx(const HVX_Vector vx) {
  // input is 0..0xffff representing 0.0  .. 1.0
  HVX_Vector p = Q6_Vh_vlut4_VuhPh(vx, 0x289A2269170E079Eull);
  p = Q6_Vh_vmps_VhVhVuhPuh_sat(p, vx, 0xF3E0D886B6D1A019ull);
  p = Q6_Vh_vmpa_VhVhVuhPuh_sat(p, vx, 0x2B1516D605F60018ull);
  p = Q6_Vh_vmpa_VhVhVuhPuh_sat(p, vx, 0x779B7CA07F7A8001ull);
  return p; // signed result, 14 fractional bits
}

// very large values overflow to look either like 0 or right below 2pi.
// the cos is then either 1 (or very close) or 0 (or very close).
// this is normal behavior for all implementations:
// large floats mod 2pi will never be accurate.
template <bool isCos>
RIPPLE_INTRIN_INLINE HVX_Vector _ripple_internal_trig_func(HVX_Vector input) {
  const HVX_Vector inv2pi = Q6_V_vsplat_R(0x3e22f983);

  // 16 bits, uint16_t, mod2pi
  input = _ripple_quantized_remainder_hf(input, inv2pi);

  // top 2 bits tell us which quadrant we are in
  const HVX_Vector quadrant_mask = Q6_Vh_vsplat_R(0xc000);
  // This is "1", for a 14-bit (sub-1) quantization:
  const HVX_Vector one_bit14 = Q6_Vh_vsplat_R(0x3fff);

  HVX_Vector quadrant = Q6_V_vand_VV(input, quadrant_mask);
  quadrant = Q6_Vuh_vlsr_VuhR(quadrant, 14);

  // input is now a 14 bit representation of its value mod pi/2
  input = Q6_V_vand_VV(input, Q6_V_vnot_V(quadrant_mask));

  const HVX_Vector const3 = Q6_Vh_vsplat_R(3);
  const HVX_Vector const2 = Q6_Vh_vsplat_R(2);
  const HVX_Vector const1 = Q6_Vh_vsplat_R(1);

  HVX_Vector sign_bit;
  // not using const1, but right below const1.
  // that value will get quantized into "1", later.
  HVX_Vector input_flipped = Q6_Vh_vsub_VhVh(one_bit14, input);

  if constexpr (isCos) {
    // cosine is negative if value is in quadrants 1 or 2
    // cosine is 'flipped' (starts at 0, goes to 1) in Q1 or Q3
    HVX_VectorPred q1 = Q6_Q_vcmp_eq_VhVh(quadrant, const1);
    HVX_VectorPred isNeg = Q6_Q_or_QQ(q1, Q6_Q_vcmp_eq_VhVh(quadrant, const2));
    sign_bit = Q6_V_vmux_QVV(isNeg, Q6_Vh_vsplat_R(0x8000), Q6_V_vzero());
    HVX_VectorPred should_flip =
        Q6_Q_or_QQ(q1, Q6_Q_vcmp_eq_VhVh(quadrant, const3));
    input = Q6_V_vmux_QVV(should_flip, input_flipped, input);
  } else {
    // set sign bit if quadrnt is 2 or 3, so > 1; otherwise, sign bit stays 0
    sign_bit = Q6_V_vmux_QVV(Q6_Q_vcmp_gt_VhVh(quadrant, const1),
                             Q6_Vh_vsplat_R(0x8000), Q6_V_vzero());
    // flip iff the quadrant is 0 or 2 (acts opposite to cosine there)
    HVX_VectorPred should_flip =
        Q6_Q_or_QQ(Q6_Q_vcmp_eq_VhVh(quadrant, Q6_V_vzero()),
                   Q6_Q_vcmp_eq_VhVh(quadrant, const2));
    input = Q6_V_vmux_QVV(should_flip, input_flipped, input);
  }

  // from u14 -> u16 -> cos (u14 result) -> hf
  HVX_Vector retval = _ripple_requant_u14_to_hf(
      _ripple_cospiby2x_approx(Q6_Vh_vasl_VhR(input, 2)));
  return Q6_V_vor_VV(retval, sign_bit);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_sinf16_hvx(HVX_Vector input) {
  return _ripple_internal_trig_func<false>(input);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_cosf16_hvx(HVX_Vector input) {
  return _ripple_internal_trig_func<true>(input);
}

// The point of this code is to skip using a vmux, which are always expensive.
// This is a shortcut.
//
// The input to this function is u16 quantized (0=0, 0x10000 = 1) modulus pi.
// The output is u15 quantized modulus pi / 2.
//
// Normally, you can get this by removing the most significant bit, but there is
// a catch: for values >= pi/2, we actually want to "flip" the 15 other bits.
// So, what we want is
//
//     0x8000 -> 0x7fff
//     0x8001 -> 0x7ffe
//           ....
//     0xfffe -> 1
//     0xffff -> 0
//
// The reason behind this is that we want to use _ripple_cospiby2x_approx for
// all inputs, and the behavior of the (absolute value of) cos on the other side
// of pi/2 is flipped from before pi / 2
//
// Now, ordinarily, such an operation can be done with an HVX_VectorPred
// variable and a vmux, but those are relatively slow; this function let's us
// skip that.
RIPPLE_INTRIN_INLINE HVX_Vector
_ripple_internal_tanf16_trick(HVX_Vector input) {
  const HVX_Vector constVMAX = Q6_Vh_vsplat_R(0xFFFF);
  return Q6_Vuh_vmin_VuhVuh(input, Q6_Vuh_vsub_VuhVuh_sat(constVMAX, input));
}

template <bool isNotCot = true>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_tanf16_hvx(HVX_Vector input) {
  const HVX_Vector inv_pi = Q6_V_vsplat_R(0x3ea2f983); // 1/pi
  // 16 bits, uint16_t, mod pi
  input = _ripple_quantized_remainder_hf(input, inv_pi);

  const HVX_Vector sign_bit = Q6_V_vand_VV(input, Q6_Vh_vsplat_R(0x8000));
  // u15, mod pi/2 + trick
  input = _ripple_internal_tanf16_trick(input);

  // value of the cosine, including sign
  // u15 -> u16 -> cosine (u14) -> hf -> signed hf
  HVX_Vector cos_val =
      Q6_V_vor_VV(_ripple_requant_u14_to_hf(
                      _ripple_cospiby2x_approx(Q6_Vh_vasl_VhR(input, 1))),
                  sign_bit);

  // now, same for the sine; no sign_bit because it's always positive between 0
  // and pi.
  input = Q6_Vh_vsub_VhVh(Q6_V_vsplat_R(0x7fff), input);
  // u15 -> u16 -> sine (u14) -> hf
  HVX_Vector sin_val = _ripple_requant_u14_to_hf(
      _ripple_cospiby2x_approx(Q6_Vh_vasl_VhR(input, 1)));

  if constexpr (isNotCot)
    return ripple_div_fp16_promote_fast(sin_val, cos_val);
  else
    return ripple_div_fp16_promote_fast(cos_val, sin_val);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_cotf16_hvx(HVX_Vector input) {
  return ripple_tanf16_hvx<false>(input);
}

#endif
