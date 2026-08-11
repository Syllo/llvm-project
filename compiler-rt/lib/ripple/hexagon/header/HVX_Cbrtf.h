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
#error "HVX_Cbrtf.h requires -mhvx-length=128B"
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
float cbrtf(float);
#ifdef __cplusplus
}
#endif

// Returns true only if the bitwise representation of an ieee754 fp32 number is
// zero, infinity, or NaN.
RIPPLE_INTRIN_INLINE int __ripple_cbrt_is_zero_inf_or_nan(unsigned int bits) {
  unsigned int abs_bits = bits & 0x7fffffffU;
  return abs_bits == 0U || abs_bits >= 0x7f800000U;
}

// Returns floor(x / 3) using a reciprocal multiply, avoiding a division.
RIPPLE_INTRIN_INLINE unsigned int __ripple_cbrt_udiv3(unsigned int x) {
  return (unsigned int)(((unsigned long long)x * 0xaaaaaaabULL) >> 33);
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_cbrtf(f32t32 vx) {
  // High-level overview.
  // 1. Preserve zero, infinity, and NaN; compute on abs(x) for finite inputs.
  // 2. Scale subnormals by 2^72, then scale the result back by 2^-24.
  // 3. Use a bit-level estimate for inverse cbrt(abs(x)) -> guess for Newton
  //    Raphson.
  // 4. Use 4 Newton Raphson iterations to obtain inverse cube root within 6ULP.
  // 5. Account for sign of x.
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  const float TWO_POWER_72 = 0x1p72f;
  const float TWO_POWER_MINUS_24 = 0x1p-24f;

  unsigned int xbits;
  __builtin_memcpy(&xbits, &x, sizeof(xbits));
  // abits: binary repr of fabs(x).
  unsigned int abits = xbits & 0x7fffffffU;

  float result = x;
  // Special cases: preserve signed zero, infinities, and NaN payloads.
  if (!__ripple_cbrt_is_zero_inf_or_nan(xbits)) {
    float ax;
    __builtin_memcpy(&ax, &abits, sizeof(ax));

    // Handle subnormals: scale up, compute, scale back.
    float scale = ripple_broadcast(BS, 0b1, 1.0f);
    if (abits < 0x00800000U) {
      ax *= TWO_POWER_72;
      scale = ripple_broadcast(BS, 0b1, TWO_POWER_MINUS_24);
      __builtin_memcpy(&abits, &ax, sizeof(abits));
    }

    // Bit-level initial approximation for inverse cbrt(abs(x)), similar
    // to quake-III's approximation of rsqrt(x).
    unsigned int tbits = 0x54a2fa8cU - __ripple_cbrt_udiv3(abits);
    float t;
    __builtin_memcpy(&t, &tbits, sizeof(t));

    // Use Newton Raphson for (1/t^3 - x) = 0, to get
    // the inverse cbrt.
    const float ONE_THIRD = 0.33333333333333331483f;
    t = t * (4.0f - ax * t * t * t) * ONE_THIRD;
    t = t * (4.0f - ax * t * t * t) * ONE_THIRD;
    t = t * (4.0f - ax * t * t * t) * ONE_THIRD;
    t = t * (4.0f - ax * t * t * t) * ONE_THIRD;

    result = ax * t * t * scale;
    if (xbits & 0x80000000U) { // get the sign bit.
      result = -result;
    }
  }

  return _RIPPLE_TO_VEC(32, float, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
