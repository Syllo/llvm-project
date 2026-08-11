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
#error "HVX_Sqrtf.h requires -mhvx-length=128B"
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
float sqrtf(float);
#ifdef __cplusplus
}
#endif

// returns true only if the bitwise representation of a ieee754 fp32 number is
// NaN or +inf.
RIPPLE_INTRIN_INLINE int __ripple_sqrt_is_posinf_or_nan(unsigned int bits) {
  return bits == 0x7f800000U || (bits & 0x7fffffffU) > 0x7f800000U;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_sqrtf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  const float TWO_POWER_64 = 0x1p64f;
  const float TWO_POWER_MINUS_32 = 0x1p-32f;

  unsigned int xbits;
  __builtin_memcpy(&xbits, &x, sizeof(xbits));

  float result = 0;
  // Special cases
  if (x < 0.0f) {
    result = __builtin_nanf("");
  } else if (x == 0.0f || __ripple_sqrt_is_posinf_or_nan(xbits)) {
    // ensures that -0 returns -0 and +0 returns +0
    result = x;
  } else {
    // Handle subnormals: scale up, compute, scale back
    float scale = ripple_broadcast(BS, 0b1, 1.0f);
    if (x < 1e-19f) { // likely subnormal or near-subnormal
      x *= TWO_POWER_64;
      scale = ripple_broadcast(BS, 0b1, TWO_POWER_MINUS_32);
    }

    __builtin_memcpy(&xbits, &x, sizeof(xbits));

    // Evil FP bit-level hacking from Quake III (from quake3/code/game/q_math.c)
    xbits = 0x5F3759DFu - (xbits >> 1);
    float y;
    __builtin_memcpy(&y, &xbits, 4);

    // 3 Newton Raphson iterations for sqrt
    float x2 = x * 0.5f;
    y = y * (1.5f - x2 * y * y);
    y = y * (1.5f - x2 * y * y);
    y = y * (1.5f - x2 * y * y); // 3rd iteration for < 4 ULP

    result = (x * y) * scale;
  }

  return _RIPPLE_TO_VEC(32, float, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
