//===-------------------------- HVX_Bfloat.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access the bfloat conversion API from Ripple.
//===------------------------------------------------------------------------===

#pragma once
#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdbool.h>
#include <stdint.h>

#if __has_bf16__

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

/// \brief Bare truncation from fp32 to bf16:
///  - assumes that the elements of `in` are non-NaNs.
///  - Gets a sub-optimal rounding error from just truncating
__bf16 to_bf16_trunc(float in);

/// \brief fp32 -> bf16 truncation that's aware of NaNs.
/// Use if there is a possibility that `in`
/// contains NaNs, but if you don't care about the rounding error introduced
/// by a bare truncation.
__bf16 to_bf16_nan(float in);

/// \brief most accurate fp32 --> bf16 conversion.
/// Uses even rounding and is aware of NaN conversions.
__bf16 to_bf16_round(float vin);

#ifdef __cplusplus
} // extern "C"
#endif

// ripple_vector_candidate definitions.

#define __hvx_bfloat_split(x, oe)                                              \
  Q6_Vh_vpack##oe##_VwVw(Q6_V_hi_W((x)), Q6_V_lo_W((x)))

RIPPLE_CANDIDATE_INLINE v64bf16 ripple_ew_pure_to_bf16_trunc(v64f32 In) {
  __bf16 ForType = 0;
  return hvx_cast_from_i32(__hvx_bfloat_split(hvx_cast_to_i32(In), o), ForType);
}

#define __hvx_bfloat_shiftNan(V64I32In, b)                                     \
  ({                                                                           \
    uint16_t ForType = 0;                                                      \
    v64u16 Trunc =                                                             \
        hvx_cast_from_i32(__hvx_bfloat_split((V64I32In), o), ForType);         \
    v64u16 CutMant =                                                           \
        hvx_cast_from_i32(__hvx_bfloat_split((V64I32In), e), ForType);         \
    uint16_t Truncated = vec_to_ripple<64, uint16_t>((b), Trunc);              \
    uint16_t CutMantissa = vec_to_ripple<64, uint16_t>((b), CutMant);          \
    constexpr uint16_t Signless = 0x7FFF;                                      \
    constexpr uint16_t Inf = 0x7F80;                                           \
    constexpr uint16_t U16Nan = 0x7FC0;                                        \
    uint16_t Sign = 0x8000 & Truncated;                                        \
    uint16_t SignedNan = U16Nan | Sign;                                        \
    bool TruncsToInf = (Signless & Truncated) == Inf;                          \
    bool NonZeroCutMantissa = CutMantissa != 0;                                \
    uint16_t TruncWithNan =                                                    \
        (TruncsToInf & NonZeroCutMantissa) ? SignedNan : Truncated;            \
    __bf16 Result;                                                             \
    __builtin_memcpy(&Result, &TruncWithNan, sizeof(Result));                  \
    ripple_to_vec<64, __bf16>(b, Result);                                      \
  })

RIPPLE_CANDIDATE_INLINE v64bf16 ripple_ew_pure_to_bf16_nan(v64f32 Vin) {
  ripple_block_t b = ripple_set_block_shape(0, 64);
  return __hvx_bfloat_shiftNan(hvx_cast_to_i32(Vin), b);
}

RIPPLE_CANDIDATE_INLINE v64bf16 ripple_ew_pure_to_bf16_round(v64f32 Vin) {
  constexpr uint32_t OxeightOOO = 0x00008000u;
  constexpr uint32_t Oxonef = 0x0001FFFF;
  ripple_block_t b = ripple_set_block_shape(0, 64);
  float In = vec_to_ripple<64, float>(b, Vin);
  uint32_t bits;
  __builtin_memcpy(&bits, &In, sizeof(bits));
  if (((bits & Oxonef) != OxeightOOO) & ((bits & OxeightOOO) == OxeightOOO) &
      ((bits & 0x7F800000) != 0x7F800000)) {
    bits += OxeightOOO;
  }
  v64i32 PreRounded = hvx_cast_to_i32(ripple_to_vec<64, uint32_t>(b, bits));
  return __hvx_bfloat_shiftNan(PreRounded, b);
}

#undef __hvx_bfloat_split
#undef __hvx_bfloat_shiftNan

#endif // __has_bf16__
