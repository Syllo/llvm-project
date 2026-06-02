//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//
// Part of the Ripple vector library to support the HVX ldexp/frexp functions.
//
//==============================================================================

#pragma once

#ifndef __cplusplus
#error "HVX_ldexp_frexp.h requires C++"
#endif

#include "__ripple_func_attribs.h"
#include <hexagon_protos.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stdint.h>

#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_ldexp_frexp.h requires -mhvx-length=128B"
#endif

// {{{ frexp

RIPPLE_INTRIN_INLINE float __ripple_frexpf(float num, int32_t *exp) {
  uint32_t num_binary;
  __builtin_memcpy(&num_binary, &num, sizeof(num_binary));

  uint32_t mantissa = num_binary & ((1u << 23) - 1);
  int32_t num_exp = (int32_t)((num_binary >> 23) & 0xFFu) - 127;
  uint32_t sign_bit = num_binary & (1u << 31);

  if (num == 0.0f) {
    *exp = 0;
    return num;
  }

  if (num_exp > 127) {
    // NaN or Infinity
    *exp = 0;
    return num;
  }

  uint32_t result_binary;

  if (num_exp == -127) {
    // subnormals
    int32_t lz = __builtin_clz(mantissa) - 9; // disregard leading 8 bits in exp
                                              // + 1 sign bit.

    *exp = -126 - lz;
    result_binary =
        sign_bit | (126u << 23) | ((mantissa << (lz + 1)) & 0x7FFFFFu);
  } else {
    *exp = num_exp + 1;
    result_binary = sign_bit | (126u << 23) | mantissa;
  }
  float result;
  __builtin_memcpy(&result, &result_binary, sizeof(result));
  return result;
}

RIPPLE_CANDIDATE_INLINE f32t32
ripple_ret_t32f32_t32f32_t32ptr_frexpf(f32t32 vnum, i32t32 *vexp) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float num = vec_to_ripple<32, float>(BS, vnum);
  int32_t exp = ripple_broadcast(BS, 0b1, 0);
  float fraction = __ripple_frexpf(num, &exp);
  *vexp = ripple_to_vec<32, int32_t>(BS, exp);
  return ripple_to_vec<32, float>(BS, fraction);
}

RIPPLE_INTRIN_INLINE _Float16 __ripple_frexpf16(_Float16 num, int16_t *exp) {
  uint16_t num_binary;
  __builtin_memcpy(&num_binary, &num, sizeof(num_binary));

  uint32_t mantissa = num_binary & ((1u << 10) - 1);
  int32_t num_exp = (int32_t)((num_binary >> 10) & 0x1Fu) - 15;
  uint16_t sign_bit = num_binary & (1u << 15);

  if (num == (_Float16)0.0) {
    *exp = 0;
    return num;
  }

  if (num_exp > 15) {
    // num is NaN or Infinity
    *exp = 0;
    return num;
  }

  uint16_t result_binary;

  if (num_exp == -15) {
    // subnormals
    int32_t lz =
        __builtin_clz(mantissa) - 22; // mantissa is stored in the low 10
                                      // bits of a 32-bit value, so clz
                                      // always counts >= 22 leading zeros
                                      // before reaching the mantissa bits.

    *exp = (int16_t)(-14 - lz);
    result_binary = sign_bit | (14u << 10) | ((mantissa << (lz + 1)) & 0x3FFu);
  } else {
    *exp = (int16_t)(num_exp + 1);
    result_binary = sign_bit | (14u << 10) | mantissa;
  }

  _Float16 result;
  __builtin_memcpy(&result, &result_binary, sizeof(result));
  return result;
}

RIPPLE_CANDIDATE_INLINE f16t64
ripple_ret_t64f16_t64f16_t64ptr_frexpf16(f16t64 vnum, i32t64 *vexp) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 num = vec_to_ripple<64, _Float16>(BS, vnum);
  int16_t exp = ripple_broadcast(BS, 0b1, 0);
  _Float16 fraction = __ripple_frexpf16(num, &exp);
  *vexp = ripple_to_vec<64, int32_t>(BS, static_cast<int32_t>(exp));
  return ripple_to_vec<64, _Float16>(BS, fraction);
}

// }}}

// {{{ ldexp

RIPPLE_INTRIN_INLINE float __ripple_ldexpf(float num, int32_t exp) {
  uint32_t num_binary;
  __builtin_memcpy(&num_binary, &num, sizeof(num_binary));
  uint32_t mantissa = num_binary & ((1u << 23) - 1);
  int32_t old_exp = (int32_t)((num_binary >> 23) & 0xFFu) - 127;
  uint32_t sign_bit = num_binary & (1u << 31);

  if (old_exp == -127 && mantissa == 0) {
    // +/-0: ldexp(+/-0, exp) == +/-0
    return num;
  }

  if (old_exp == -127) {
    // num is subnormal: renormalize so old_exp reflects the true exponent
    // of the (now implicit-leading-1) mantissa. Without this, ldexp on a
    // subnormal whose result becomes normal is encoded incorrectly.
    int32_t lz = __builtin_clz(mantissa) - 9; // disregard leading 8 exp bits
                                              // + 1 sign bit.
    old_exp = -126 - lz;
    mantissa = (mantissa << (lz + 1)) & 0x7FFFFFu;
  }

  int32_t new_exp = ripple_add_sat(old_exp, exp);

  uint32_t result_binary;
  if (old_exp > 127) {
    // num is NaN or Infinity
    result_binary = num_binary;
  } else if (new_exp > 127) {
    // num was finite and (num * 2^exp) is infinite
    result_binary = (0xFFu << 23) | sign_bit;
  } else if (new_exp < -126) {
    // (num * 2^exp) is subnormal
    int32_t shift = -126 - new_exp;
    result_binary =
        (shift < 24 ? ((mantissa | (1u << 23)) >> shift) : 0u) | sign_bit;
  } else {
    // both num and (num * 2^exp) are finite.
    result_binary = sign_bit | (((uint32_t)(new_exp + 127)) << 23) | mantissa;
  }
  float result;
  __builtin_memcpy(&result, &result_binary, sizeof(float));
  return result;
}

RIPPLE_CANDIDATE_INLINE f32t32 ripple_ew_pure_ldexpf(f32t32 vnum, i32t32 vexp) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  float num = vec_to_ripple<32, float>(BS, vnum);
  int32_t exp = vec_to_ripple<32, int32_t>(BS, vexp);
  return ripple_to_vec<32, float>(BS, __ripple_ldexpf(num, exp));
}

RIPPLE_INTRIN_INLINE _Float16 __ripple_ldexpf16(_Float16 num, int16_t exp) {
  uint16_t num_binary;
  __builtin_memcpy(&num_binary, &num, sizeof(num_binary));
  uint16_t mantissa = num_binary & ((1u << 10) - 1);
  int16_t old_exp = (int16_t)((num_binary >> 10) & 0x1Fu) - 15;
  uint16_t sign_bit = num_binary & (1u << 15);

  if (old_exp == -15 && mantissa == 0) {
    // +/-0: ldexp(+/-0, exp) == +/-0
    return num;
  }

  if (old_exp == -15) {
    // num is subnormal: renormalize so old_exp reflects the true exponent
    // of the (now implicit-leading-1) mantissa. Without this, ldexp on a
    // subnormal whose result becomes normal is encoded incorrectly.
    int16_t lz = (int16_t)(__builtin_clz(mantissa) - 22); // mantissa is stored
                                                          // in the low 10 bits
                                                          // of a 32-bit value.
    old_exp = (int16_t)(-14 - lz);
    mantissa = (uint16_t)((mantissa << (lz + 1)) & 0x3FFu);
  }

  int16_t new_exp = ripple_add_sat(old_exp, exp);

  uint16_t result_binary;
  if (old_exp > 15) {
    // num is NaN or Infinity
    result_binary = num_binary;
  } else if (new_exp > 15) {
    // num was finite and (num * 2^exp) is infinite
    result_binary = (0x1Fu << 10) | sign_bit; // INF with num's sign
  } else if (new_exp < -14) {
    // (num * 2^exp) is subnormal
    int16_t shift = -14 - new_exp;
    result_binary =
        (shift < 11 ? (uint16_t)((mantissa | (1u << 10)) >> shift) : 0u) |
        sign_bit;
  } else {
    // both num and (num * 2^exp) are finite.
    result_binary = sign_bit | (((uint16_t)(new_exp + 15)) << 10) | mantissa;
  }
  _Float16 result;
  __builtin_memcpy(&result, &result_binary, sizeof(result));
  return result;
}

RIPPLE_CANDIDATE_INLINE f16t64 ripple_ew_pure_ldexpf16(f16t64 vnum,
                                                       i32t64 vexp) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  _Float16 num = vec_to_ripple<64, _Float16>(BS, vnum);
  i16t64 vexph = Q6_Vh_vpack_VwVw_sat(Q6_V_hi_W(vexp), Q6_V_lo_W(vexp));
  int16_t exp = vec_to_ripple<64, int16_t>(BS, vexph);
  return ripple_to_vec<64, _Float16>(BS, __ripple_ldexpf16(num, exp));
}

// }}}
