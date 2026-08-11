//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdint.h>

// Candidate bodies use 128-byte HVX vectors and require -fenable-ripple.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_MathTrigfp16.h requires -mhvx-length=128B"
#endif

// The vector conversion helpers are templates in C++ and macros in C.
#ifdef __cplusplus
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec<N, T>((BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple<N, T>((BS), (X))
#else
#define _RIPPLE_TO_VEC(N, T, BS, X) ripple_to_vec(N, T, (BS), (X))
#define _VEC_TO_RIPPLE(N, T, BS, X) vec_to_ripple(N, T, (BS), (X))
#endif

#ifdef __cplusplus
extern "C" {
#endif
_Float16 sinf16(_Float16);
_Float16 cosf16(_Float16);
_Float16 tanf16(_Float16);
#ifdef __cplusplus
}
#endif

RIPPLE_INTRIN_INLINE _Float16
__ripple_trig16_cody_waite_range_redn(_Float16 xh, short *quadrant) {
  const float inv_pio2 = 0.63661977236758134308f;
  const float pio2_hi = 1.5703125f;
  const float pio2_lo = 0.00048382679489661923f;

  float x = (float)xh;
  int n = (int)(x * inv_pio2 + (x >= 0.0f ? 0.5f : -0.5f));
  float n_float = (float)n;
  float product = n_float * pio2_hi;
  float y = x - product;
  y = y - n_float * pio2_lo;

  *quadrant = (short)((unsigned int)n & 0x3u);
  return (_Float16)y;
}

RIPPLE_INTRIN_INLINE _Float16
__ripple_trig16_sinf_on_absx_le_pi_by_4(_Float16 x) {
  // Coefficients computed using Sollya
  // Run with: `sollya file.sollya` to find a fp16-coefficient polynomial P
  // such that x * P(x) approximates sin(x) on [-pi/4,pi/4] with
  // inf_norm(rtol) < 3.9e-3 (before evaluation rounding).
  // -------------------- file.sollya ----------------------------
  // prec = 200!;
  // i = [-pi/4; pi/4];
  // f = sin(x) / x;
  // p = fpminimax(f, [|0,2|], [|HP...|], i, relative);
  // print("polynomial P(x):", p);
  // print("relative error:", dirtyinfnorm(p/f-1, i));
  // ----------------------------------------------------------------
  const _Float16 c1 = (_Float16)0.99951171875f;
  const _Float16 c3 = (_Float16)-0.161376953125f;

  _Float16 xsq = x * x;
  return x * (xsq * c3 + c1);
}

RIPPLE_INTRIN_INLINE _Float16
__ripple_trig16_cosf_on_absx_le_pi_by_4(_Float16 x) {
  // Coefficients computed using Sollya
  // Run with: `sollya file.sollya` to find a fp16-coefficient polynomial
  // for cos(x) on [-pi/4,pi/4] to have inf_norm(rtol) < 3.9e-3
  // (before evaluation rounding).
  // -------------------- file.sollya ----------------------------
  // prec = 200!;
  // i = [-pi/4; pi/4];
  // f = cos(x);
  // p = fpminimax(f, [|0,2|], [|HP...|], i, relative);
  // print("polynomial P(x):", p);
  // print("relative error:", dirtyinfnorm(p/f-1, i));
  // ----------------------------------------------------------------
  const _Float16 c0 = (_Float16)0.99755859375f;
  const _Float16 c2 = (_Float16)-0.473388671875f;

  _Float16 xsq = x * x;
  return xsq * c2 + c0;
}

RIPPLE_INTRIN_INLINE _Float16
__ripple_trig16_tanf_on_absx_le_pi_by_4(_Float16 x) {
  // Coefficients computed using Sollya
  // Run with: `sollya file.sollya` to find a fp16-coefficient polynomial P
  // such that x * P(x) approximates tan(x) on [-pi/4,pi/4] with
  // inf_norm(rtol) < 3.9e-3 (before evaluation rounding).
  // -------------------- file.sollya ----------------------------
  // prec = 200!;
  // i = [-pi/4; pi/4];
  // f = tan(x) / x;
  // p = fpminimax(f, [|0,2,4|], [|HP...|], i, relative);
  // print("polynomial P(x):", p);
  // print("relative error:", dirtyinfnorm(p/f-1, i));
  // ----------------------------------------------------------------
  const _Float16 c1 = (_Float16)1.0009765625f;
  const _Float16 c3 = (_Float16)0.3134765625f;
  const _Float16 c5 = (_Float16)0.2054443359375f;

  _Float16 xsq = x * x;
  _Float16 y = xsq * c5 + c3;
  return x * (xsq * y + c1);
}

RIPPLE_INTRIN_INLINE _Float16 __ripple_trig16_recipf16(_Float16 x) {
  // Three iterations bring this bit-hack seed within four ulps for the
  // reduced cotangent input range.
  unsigned short x_bits;
  __builtin_memcpy(&x_bits, &x, sizeof(x_bits));

  const unsigned short r_bits = (unsigned short)(0x7942u - x_bits);
  _Float16 r;
  __builtin_memcpy(&r, &r_bits, sizeof(r));

  r = r * ((_Float16)2 - x * r);
  r = r * ((_Float16)2 - x * r);
  return r * ((_Float16)2 - x * r);
}

RIPPLE_INTRIN_INLINE _Float16
__ripple_trig16_cotf_on_absx_le_pi_by_4(_Float16 x) {
  // Coefficients computed using Sollya
  // Run with: `sollya file.sollya` to find a fp16-coefficient polynomial P
  // such that P(x) / x approximates cot(x) on [-pi/4,pi/4] with
  // inf_norm(rtol) < 3.9e-3 (before evaluation rounding).
  // -------------------- file.sollya ----------------------------
  // prec = 200!;
  // i = [-pi/4; pi/4];
  // f = x / tan(x);
  // p = fpminimax(f, [|0,2|], [|HP...|], i, relative);
  // print("polynomial P(x):", p);
  // print("relative error:", dirtyinfnorm(p/f-1, i));
  // ----------------------------------------------------------------
  const _Float16 c0 = (_Float16)1.0009765625f;
  const _Float16 c2 = (_Float16)-0.34765625f;

  _Float16 xsq = x * x;
  _Float16 p = xsq * c2 + c0;
  return p * __ripple_trig16_recipf16(x);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_sinf16(f16t64 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  short quadrant = ripple_broadcast(BS, 0b1, (short)0);
  _Float16 r = __ripple_trig16_cody_waite_range_redn(x, &quadrant);

  // Select the reduced polynomial and sign from the quadrant bits.
  // Two selects also avoid a four-way branch in header candidates.
  _Float16 sine = __ripple_trig16_sinf_on_absx_le_pi_by_4(r);
  _Float16 cosine = __ripple_trig16_cosf_on_absx_le_pi_by_4(r);
  _Float16 y = (quadrant & 1) ? cosine : sine;
  y = (quadrant & 2) ? -y : y;

  return _RIPPLE_TO_VEC(64, _Float16, BS, y);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_cosf16(f16t64 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  short quadrant = ripple_broadcast(BS, 0b1, (short)0);
  _Float16 r = __ripple_trig16_cody_waite_range_redn(x, &quadrant);
  // Select the reduced polynomial and sign from the quadrant bits.
  // Two selects also avoid a four-way branch in header candidates.
  _Float16 sine = __ripple_trig16_sinf_on_absx_le_pi_by_4(r);
  _Float16 cosine = __ripple_trig16_cosf_on_absx_le_pi_by_4(r);
  _Float16 y = (quadrant & 1) ? sine : cosine;
  y = ((quadrant + 1) & 2) ? -y : y;
  return _RIPPLE_TO_VEC(64, _Float16, BS, y);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_tanf16(f16t64 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  short quadrant = ripple_broadcast(BS, 0b1, (short)0);
  _Float16 r = __ripple_trig16_cody_waite_range_redn(x, &quadrant);
  _Float16 y = ripple_broadcast(BS, 0b1, (_Float16)0.f);

  if ((quadrant & 1) == 0) {
    y = __ripple_trig16_tanf_on_absx_le_pi_by_4(r);
  } else {
    y = -__ripple_trig16_cotf_on_absx_le_pi_by_4(r);
  }
  return _RIPPLE_TO_VEC(64, _Float16, BS, y);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
