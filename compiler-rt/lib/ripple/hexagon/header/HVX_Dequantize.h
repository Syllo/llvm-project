//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//

#pragma once

#ifndef __cplusplus
#error "HVX_Dequantize.h requires C++"
#endif

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdbool.h>
#include <stdint.h>

// Required compile flags (previously enforced per-source by the bitcode build,
// now the includer's responsibility in the header-only model):
//   -mhvx-length=128B   : the candidate bodies assume 128-byte HVX vectors.
//   On arch >= v73, also -mllvm -hexagon-fp-fast-convert=true to match the
//   float-conversion semantics the library was validated against.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_Dequantize.h requires -mhvx-length=128B"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

uint32_t hvx_dequantize_u8_to_f32(const uint8_t vin, const int16_t zero_offset,
                                  const uint8_t from_qint8,
                                  const int32_t iscale, const int16_t exp_base);
float hvx_dequantize_u16_to_f32_flat(const uint16_t vin, uint32_t offset,
                                     float scale);
uint32_t hvx_dequantize_u16_to_f32(const uint16_t vin, uint16_t zero_offset,
                                   uint16_t from_qint16, int32_t iscale,
                                   int16_t exp_base);

#ifdef __cplusplus
} // extern "C"
#endif

// ripple_vector_candidate definitions.

RIPPLE_CANDIDATE_INLINE v128u32 ripple_pure_ew_hvx_dequantize_u8_to_f32(
    v128u8 vin, const int16_t zero_offset, const uint8_t from_qint8,
    const int32_t iscale, const int16_t exp_base) {
  auto BS = ripple_set_block_shape(0, 128);
  uint8_t in = vec_to_ripple<128, uint8_t>(BS, vin);

  uint8_t in_ub = in ^ from_qint8;

  int16_t vin_h = static_cast<int16_t>(in_ub) - zero_offset;

  int16_t vin_h_neg = -vin_h;
  int16_t inabs_h = vin_h > 0 ? vin_h : vin_h_neg;
  int32_t prod_w = iscale * static_cast<int32_t>(inabs_h);

  uint32_t clz = __builtin_clz(prod_w);
  uint16_t clz_h = static_cast<uint16_t>(clz & 0xFFFF);

  prod_w = prod_w << clz_h;

  int32_t mant = prod_w + 0x80;
  int16_t manth = static_cast<int16_t>(mant >> 16);
  mant = mant >> 8;

  uint16_t sign = vin_h & 0x8000;
  uint16_t base_with_sign = (exp_base * 128) | (sign ? 0x8000 : 0);
  uint16_t exp = base_with_sign - clz_h * 0x0080;

  bool noflo = (0 > manth) ? true : false;
  bool iszero = (vin_h == 0) ? true : false;

  uint16_t oflo_add = (!noflo) ? 0x0080 : 0;
  exp += oflo_add;
  exp = (!iszero) ? exp : 0;

  uint32_t exp_w = static_cast<uint32_t>(exp << 16);
  uint32_t res = (mant & 0x007FFFFF) | exp_w;

  return ripple_to_vec<128, uint32_t>(BS, res);
}

RIPPLE_CANDIDATE_INLINE v64f32 ripple_pure_hvx_dequantize_u16_to_f32_flat(
    const v64u16 vin, uint32_t offset, float scale) {
  auto BS = ripple_set_block_shape(0, 64);
  uint16_t in16 = vec_to_ripple<64, uint16_t>(BS, vin);
  int32_t in32 = static_cast<int32_t>(in16);
  int32_t sub32 = in32 - offset;
  float sub_f = static_cast<float>(sub32);
  return ripple_to_vec<64, float>(BS, sub_f * scale);
}

RIPPLE_CANDIDATE_INLINE v64u32 ripple_pure_hvx_dequantize_u16_to_f32(
    const v64u16 vin, const uint16_t zero_offset, const uint16_t from_qint16,
    const int32_t iscale, const int16_t exp_base) {

  auto BS = ripple_set_block_shape(0, 64);
  uint16_t in = vec_to_ripple<64, uint16_t>(BS, vin);

  uint16_t in_uh = in ^ from_qint16;
  uint16_t rzero_offset = ripple_broadcast(BS, 1, zero_offset);
  v64u16 vin_uh = ripple_to_vec<64, uint16_t>(BS, in_uh);
  v64u16 vzero_offset = ripple_to_vec<64, uint16_t>(BS, rzero_offset);
  v64u16 vmag_uh = Q6_Vuh_vabsdiff_VuhVuh(vin_uh, vzero_offset);
  uint16_t mag_uh = vec_to_ripple<64, uint16_t>(BS, vmag_uh);
  uint16_t q_is_zero_h = (zero_offset == in_uh) ? 1 : 0;
  uint16_t sign_uh = (zero_offset > in_uh) ? 0x8000 : 0x0000;

  uint16_t normshift_uh = __builtin_clz(mag_uh << 16);
  mag_uh = mag_uh << normshift_uh;
  int16_t exp_h = exp_base - normshift_uh;

  int32_t mag_w = static_cast<int32_t>(
      (static_cast<int64_t>(iscale) * static_cast<int64_t>(mag_uh)) >> 16);

  int32_t small_w = (0x3fffffe0 > mag_w) ? 1 : 0;
  int32_t scale_w = 0x01000000 << small_w;

  v64i32 vscale_w = ripple_to_vec<64, int32_t>(BS, scale_w);
  v64i32 vmag_w = ripple_to_vec<64, int32_t>(BS, mag_w);
  v32i32 vmant0_w =
      Q6_Vw_vmpyo_VwVh_s1_rnd_sat(Q6_V_lo_W(vmag_w), Q6_V_lo_W(vscale_w));
  v32i32 vmant1_w =
      Q6_Vw_vmpyo_VwVh_s1_rnd_sat(Q6_V_hi_W(vmag_w), Q6_V_hi_W(vscale_w));
  v64i32 vmant_w = Q6_W_vcombine_VV(vmant1_w, vmant0_w);
  int32_t mant_w = vec_to_ripple<64, int32_t>(BS, vmant_w);
  int32_t k_007F_FFFF = 0x007FFFFF;
  mant_w = mant_w & ripple_broadcast(BS, 1, k_007F_FFFF);

  int16_t nzmask_h = q_is_zero_h == 1 ? 1 : -1;

  exp_h -= small_w;
  exp_h = (exp_h < 0x00FF) ? exp_h : 0x00FF;
  exp_h = exp_h & nzmask_h;
  int32_t nzmask_w = (0x00FF > exp_h) ? nzmask_h : 0;

  uint32_t exp_w = ((exp_h << 7) + sign_uh) << 16;

  uint32_t result = (nzmask_w & mant_w) | exp_w;

  return ripple_to_vec<64, uint32_t>(BS, result);
}
