//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//
// Part of the Ripple vector library to support the HVX nextafter functions.
//
//==============================================================================

#pragma once

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>

#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_nextafter.h requires -mhvx-length=128B"
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
float nextafterf(float, float);
_Float16 nextafterf16(_Float16, _Float16);
#ifdef __cplusplus
}
#endif

// These pure candidates compute result bits without updating errno or raising
// floating-point exceptions. Integer comparisons preserve subnormal inputs.
RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_nextafterf(f32t32 vx, f32t32 vy) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = _VEC_TO_RIPPLE(32, float, BS, vy);

  float result;
  if (__builtin_isnan(x) || __builtin_isnan(y)) {
    result = ripple_broadcast(BS, 0b1, __builtin_nanf(""));
  } else {
    unsigned int x_bits;
    unsigned int y_bits;
    __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
    __builtin_memcpy(&y_bits, &y, sizeof(y_bits));
    unsigned int x_abs = x_bits & 0x7fffffffU;
    unsigned int y_abs = y_bits & 0x7fffffffU;
    // A clear sign bit counts as positive, including positive zero.
    int is_x_positive = (x_bits & 0x80000000U) == 0;
    int is_y_positive = (y_bits & 0x80000000U) == 0;
    unsigned int result_bits;

    if (x_bits == y_bits || (x_abs == 0 && y_abs == 0)) {
      // Equal values return y, including its sign when both inputs are zero.
      result_bits = y_bits;
    } else if (x_abs == 0) {
      // Step from zero to the least subnormal with y's sign.
      result_bits = is_y_positive ? 1U : (0x80000000U | 1U);
    } else {
      // Opposite signs or a smaller target magnitude require a step toward zero.
      // Carries and borrows also handle exponent boundaries and infinities.
      result_bits = (is_x_positive != is_y_positive) || x_abs > y_abs
                        ? x_bits - 1U
                        : x_bits + 1U;
    }

    __builtin_memcpy(&result, &result_bits, sizeof(result));
  }
  return _RIPPLE_TO_VEC(32, float, BS, result);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_nextafterf16(f16t64 vx,
                                                           f16t64 vy) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  _Float16 y = _VEC_TO_RIPPLE(64, _Float16, BS, vy);

  _Float16 result;
  if (__builtin_isnan(x) || __builtin_isnan(y)) {
    result = ripple_broadcast(BS, 0b1, (_Float16)__builtin_nanf(""));
  } else {
    unsigned short x_bits;
    unsigned short y_bits;
    __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
    __builtin_memcpy(&y_bits, &y, sizeof(y_bits));
    unsigned short x_abs = x_bits & 0x7fffU;
    unsigned short y_abs = y_bits & 0x7fffU;
    // A clear sign bit counts as positive, including positive zero.
    int is_x_positive = (x_bits & 0x8000U) == 0;
    int is_y_positive = (y_bits & 0x8000U) == 0;
    unsigned short result_bits;

    if (x_bits == y_bits || (x_abs == 0 && y_abs == 0)) {
      // Equal values return y, including its sign when both inputs are zero.
      result_bits = y_bits;
    } else if (x_abs == 0) {
      // Step from zero to the least subnormal with y's sign.
      result_bits = is_y_positive ? 1U : (0x8000U | 1U);
    } else {
      // Opposite signs or a smaller target magnitude require a step toward zero.
      // Carries and borrows also handle exponent boundaries and infinities.
      result_bits = (is_x_positive != is_y_positive) || x_abs > y_abs
                        ? x_bits - 1U
                        : x_bits + 1U;
    }

    __builtin_memcpy(&result, &result_bits, sizeof(result));
  }
  return _RIPPLE_TO_VEC(64, _Float16, BS, result);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
