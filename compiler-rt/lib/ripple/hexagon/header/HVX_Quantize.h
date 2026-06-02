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
#error "HVX_Quantize.h requires C++"
#endif

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdint.h>

// Required compile flags (previously enforced per-source by the bitcode build,
// now the includer's responsibility in the header-only model):
//   -mhvx-length=128B   : the candidate bodies assume 128-byte HVX vectors.
//   On arch >= v73, also -mllvm -hexagon-fp-fast-convert=true to match the
//   float-conversion semantics the library was validated against.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_Quantize.h requires -mhvx-length=128B"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

uint8_t hvx_quantize_f32_to_u8(float scale_f, int16_t out_offset,
                               const float in);
uint16_t hvx_quantize_f32_to_u16(float scale_f, int32_t out_offset,
                                 const float in);

#ifdef __cplusplus
} // extern "C"
#endif

// ripple_vector_candidate definitions.

RIPPLE_CANDIDATE_INLINE v128u8 ripple_pure_ew_hvx_quantize_f32_to_u8(
    float scale_f, int16_t out_offset, const v128f32 in) {
  auto BS = ripple_set_block_shape(0, 128);
  float xf = vec_to_ripple<128, float>(BS, in);

  xf = xf * scale_f;

  _Float16 xhf = static_cast<_Float16>(xf);

  int16_t xh = static_cast<int16_t>(xhf);

  xh = __builtin_ripple_add_sat_i16(xh, out_offset);

  v128u16 vh = ripple_to_vec<128, uint16_t>(BS, xh);
  return Q6_Vub_vpack_VhVh_sat(Q6_V_hi_W(vh), Q6_V_lo_W(vh));
}

RIPPLE_CANDIDATE_INLINE v64u16 ripple_pure_ew_hvx_quantize_f32_to_u16(
    float scale_f, int32_t out_offset, const v64f32 in) {
  auto BS = ripple_set_block_shape(0, 64);
  float xf = vec_to_ripple<64, float>(BS, in);

  xf = xf * scale_f;

  uint32_t xf_bits;
  __builtin_memcpy(&xf_bits, &xf, sizeof(xf));
  int32_t sign_bits = xf_bits & 0x80000000;
  sign_bits = sign_bits | 0x3F000000;
  float sign_f;
  __builtin_memcpy(&sign_f, &sign_bits, sizeof(sign_bits));
  xf = xf + sign_f;

  int32_t xw = static_cast<int32_t>(xf);

  xw = __builtin_ripple_add_sat_i32(xw, out_offset);

  v64u32 vw = ripple_to_vec<64, uint32_t>(BS, xw);
  return Q6_Vuh_vpack_VwVw_sat(Q6_V_hi_W(vw), Q6_V_lo_W(vw));
}
