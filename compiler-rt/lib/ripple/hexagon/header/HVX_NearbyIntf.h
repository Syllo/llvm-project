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
#error "HVX_NearbyIntf.h requires -mhvx-length=128B"
#endif

#include "ripple_trunc_round_f32_f16.h"

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
float nearbyintf(float);
_Float16 nearbyintf16(_Float16);
float rintf(float);
_Float16 rintf16(_Float16);
#ifdef __cplusplus
}
#endif

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_nearbyintf(f32t32 vx) {
  // TODO: read the rounding mode from fegetround of fenv.h, current
  // implementation rounds to nearest even.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  int is_neg = x < 0.0f;
  float abs_x = is_neg ? -x : x;
  float result = x;

  // All finite values with exponent at least 23 are already integral. This
  // also preserves infinities and NaN payloads unchanged.
  if (abs_x < 0x1p23f) {
    int integral = _VEC_TO_RIPPLE(
        32, int, BS,
        _ripple_internal_w_from_sf(_RIPPLE_TO_VEC(32, float, BS, x)));
    float fraction = x - integral;
    float abs_fraction = fraction < 0.0f ? -fraction : fraction;

    if (abs_fraction > 0.5f || (abs_fraction == 0.5f && (integral & 1) != 0))
      integral += is_neg ? -1 : 1;

    result = integral;
    if (integral == 0)
      // sign preservation for 0
      result = is_neg ? -0.0f : x * 0.0f;
  }

  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_nearbyintf16(f16t64 vx) {
  // TODO: read the rounding mode from fegetround of fenv.h, current
  // implementation rounds to nearest even.
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  int is_neg = x < (_Float16)0.0f;
  _Float16 abs_x = is_neg ? -x : x;
  _Float16 result = x;

  // All finite values with exponent at least 10 are already integral. This
  // also preserves infinities and NaN payloads unchanged.
  if (abs_x < (_Float16)0x1p10f) {
    short integral = _VEC_TO_RIPPLE(
        64, short, BS,
        _ripple_internal_h_from_hf(_RIPPLE_TO_VEC(64, _Float16, BS, x)));
    _Float16 fraction = x - integral;
    _Float16 abs_fraction = fraction < (_Float16)0.0f ? -fraction : fraction;

    if (abs_fraction > (_Float16)0.5f ||
        (abs_fraction == (_Float16)0.5f && (integral & 1) != 0))
      integral += is_neg ? -1 : 1;

    result = integral;
    if (integral == 0)
      // sign preservation for 0
      result = is_neg ? (_Float16)-0.0f : x * (_Float16)0.0f;
  }

  return _RIPPLE_TO_VEC(64, _Float16, BS, result);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_rintf(f32t32 vx) {
  return ripple_ew_pure_nearbyintf(vx);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_rintf16(f16t64 vx) {
  return ripple_ew_pure_nearbyintf16(vx);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
