//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include "__ripple_func_attribs.h"
#include <limits.h>
#include <ripple.h>
#include <ripple_hvx.h>

#ifndef FP_ILOGB0
#define FP_ILOGB0 INT_MIN
#endif
#ifndef FP_ILOGBNAN
#define FP_ILOGBNAN INT_MAX
#endif

#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_logbf.h requires -mhvx-length=128B"
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
float logbf(float);
int ilogbf(float);
#ifdef __cplusplus
}
#endif

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_logbf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);

  // High-level overview:
  // 1. Compute X = abs(x)
  // 2. If X is 0, return -INFINITY
  // 3. If X is NaN, return NAN
  // 4. If X is +-inf, return +INFINITY
  // 5. If X is normal-floating point number -> capture the exponent, shift, and
  //    offset.
  // 6. If X is subnormal-floating number -> capture exponent via counting
  //    leading zeros.

  float result = ripple_broadcast(BS, 0b1, 0);
  unsigned int x_bits;
  float X = __builtin_fabsf(x);
  __builtin_memcpy(&x_bits, &X, sizeof(x_bits));
  unsigned int exp = (x_bits >> 23) & 0xffU;

  if (X == 0.f) {
    result = ripple_broadcast(BS, 0b1, -__builtin_inff());
  } else if (__builtin_isnan(X)) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else if (__builtin_isinf(X)) {
    result = ripple_broadcast(BS, 0b1, __builtin_inff());
  } else if (exp != 0) {
    // Normal floating points
    result = (float)((int)exp - 127);
  } else {
    // X = mantissa * 2^-149; its highest set bit is 31 - clz(mantissa).
    // Bit 0 keeps clz defined for inactive zero lanes without changing it.
    result = (float)(-118 - __builtin_clz(x_bits | 1U));
  }

  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE i32t32 ripple_ew_pure_ilogbf(f32t32 vx) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);

  // High-level overview:
  // 1. Compute X = abs(x)
  // 2. If X is 0, return FP_ILOGB0
  // 3. If X is NaN, return FP_ILOGBNAN
  // 4. If X is +-inf, return INT_MAX
  // 5. If X is normal-floating point number -> capture the exponent, shift, and
  //    offset.
  // 6. If X is subnormal-floating number -> capture exponent via counting
  //    leading zeros.

  int result = ripple_broadcast(BS, 0b1, 0);
  unsigned int x_bits;
  float X = __builtin_fabsf(x);
  __builtin_memcpy(&x_bits, &X, sizeof(x_bits));
  unsigned int exp = (x_bits >> 23) & 0xffU;

  if (X == 0.f) {
    result = ripple_broadcast(BS, 0b1, FP_ILOGB0);
  } else if (__builtin_isnan(X)) {
    result = ripple_broadcast(BS, 0b1, FP_ILOGBNAN);
  } else if (__builtin_isinf(X)) {
    result = ripple_broadcast(BS, 0b1, INT_MAX);
  } else if (exp != 0) {
    // Normal floating points
    result = (int)exp - 127;
  } else {
    // X = mantissa * 2^-149; its highest set bit is 31 - clz(mantissa).
    // Bit 0 keeps clz defined for inactive zero lanes without changing it.
    result = -118 - __builtin_clz(x_bits | 1U);
  }

  return _RIPPLE_TO_VEC(32, int, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
