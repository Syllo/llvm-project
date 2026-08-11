//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//
// Part of the Ripple vector library to support the HVX copysign functions.
//
//==============================================================================

#pragma once

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>

#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_copysign.h requires -mhvx-length=128B"
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
float copysignf(float, float);
_Float16 copysignf16(_Float16, _Float16);
#ifdef __cplusplus
}
#endif

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_copysignf(f32t32 vx, f32t32 vy) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float x = _VEC_TO_RIPPLE(32, float, BS, vx);
  float y = _VEC_TO_RIPPLE(32, float, BS, vy);

  float xwithcopysign;
  unsigned int x_bits;
  unsigned int y_bits;
  __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
  __builtin_memcpy(&y_bits, &y, sizeof(y_bits));
  // Clear x's sign bit.
  x_bits = (x_bits & 0x7fffffffU);
  // Copy y's sign bit.
  x_bits |= (y_bits & 0x80000000U);

  __builtin_memcpy(&xwithcopysign, &x_bits, sizeof(xwithcopysign));
  return _RIPPLE_TO_VEC(32, float, BS, xwithcopysign);
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_copysignf16(f16t64 vx,
                                                          f16t64 vy) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 x = _VEC_TO_RIPPLE(64, _Float16, BS, vx);
  _Float16 y = _VEC_TO_RIPPLE(64, _Float16, BS, vy);

  _Float16 xwithcopysign;
  unsigned short x_bits;
  unsigned short y_bits;
  __builtin_memcpy(&x_bits, &x, sizeof(x_bits));
  __builtin_memcpy(&y_bits, &y, sizeof(y_bits));
  // Clear x's sign bit.
  x_bits = (x_bits & 0x7fffU);
  // Copy y's sign bit.
  x_bits |= (y_bits & 0x8000U);
  __builtin_memcpy(&xwithcopysign, &x_bits, sizeof(xwithcopysign));

  return _RIPPLE_TO_VEC(64, _Float16, BS, xwithcopysign);
}

#undef _RIPPLE_TO_VEC
#undef _VEC_TO_RIPPLE
