//==============================================================================
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//==============================================================================
//
// Public API declarations for the HVX in-lane vector lookup table runtime
// library.
//
// The explicitly-sized entry points (hvx_prepare_lut_* / hvx_vlut_*) build a
// prepared lookup table and perform an in-lane lookup for a given table size
// and element width.  Callers pass the table in natural order; any required
// internal preparation is handled by the runtime.  Names describe only the
// user-visible operation (entry count + element width as u8/u16/u32/u64); the
// underlying HVX instruction (vlut32 for u8, vlut16 for the wider widths) is an
// implementation detail and is not part of the public name.
//
// A polymorphic hvx_vlut(indices, prepared) / hvx_prepare_lut(table) pair is
// also provided that dispatches on the element type of the prepared table to
// the natural single-block size for that type:
//   unsigned short     / short / _Float16 / __bf16 -> 128-entry u16
//   unsigned int / int / unsigned long / long / float -> 128-entry u32
//   unsigned long long / long long / double         -> 64-entry  u64
// Floating-point element types are handled by reinterpreting (bit-casting) the
// value to the same-width unsigned integer, performing the integer lookup, and
// bit-casting the result back.

#pragma once
#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <ripple.h>
#include <ripple_hvx.h>

// The vlut candidate bodies assume 128-byte HVX vectors (block sizes and
// vlut32/vlut16 lane counts).  The old bitcode build enforced -mhvx-length=128B
// per source; in the header-only model the includer must supply it.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_Vlut.h requires -mhvx-length=128B"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

// u8 (256-/128-entry) lookups.  Implemented via the HVX vlut32 instruction.
__attribute__((always_inline)) extern unsigned char
hvx_prepare_lut_256u8(unsigned char table);

__attribute__((always_inline)) extern unsigned char
hvx_prepare_lut_128x2u8(unsigned char table);

__attribute__((always_inline)) extern unsigned char
hvx_prepare_lut_128u8(unsigned char table);

__attribute__((always_inline)) extern unsigned char
hvx_vlut_u8_w_ptr(unsigned char indices, const unsigned char *prepared_table);

__attribute__((always_inline)) extern unsigned char
hvx_vlut_u8_w_value(unsigned char indices, unsigned char table);

__attribute__((always_inline)) extern unsigned char
hvx_vlut_u8_2vec(unsigned char indices, const unsigned char table_lo,
                 const unsigned char table_hi);

// u16 lookups.  Implemented via vlut16.
__attribute__((always_inline)) extern unsigned short
hvx_prepare_lut_64u16(unsigned short table);

__attribute__((always_inline)) extern unsigned short
hvx_vlut_64u16(unsigned char indices, unsigned short table);

__attribute__((always_inline)) extern unsigned short
hvx_prepare_lut_128u16(unsigned short table);

__attribute__((always_inline)) extern unsigned short
hvx_vlut_128u16(unsigned char indices, unsigned short table);

__attribute__((always_inline)) extern unsigned short
hvx_prepare_lut_256u16(unsigned short table);

__attribute__((always_inline)) extern unsigned short
hvx_vlut_256u16(unsigned char indices, unsigned short table);

// u32 lookups, implemented via vlut16.
__attribute__((always_inline)) extern unsigned int
hvx_prepare_lut_128u32(unsigned int table);

__attribute__((always_inline)) extern unsigned int
hvx_vlut_128u32(unsigned char indices, unsigned int table);

// 256-entry u32 lookup: prepared via hvx_prepare_lut_256u32 and looked up via
// hvx_vlut_256u32.
__attribute__((always_inline)) extern unsigned int
hvx_prepare_lut_256u32(unsigned int table);

__attribute__((always_inline)) extern unsigned int
hvx_vlut_256u32(unsigned char indices, unsigned int table);

// u64 lookups, implemented via vlut16.  The element type is a 64-bit integer
// ("unsigned long long" on Hexagon, where unsigned long is 32-bit).
__attribute__((always_inline)) extern unsigned long long
hvx_prepare_lut_64u64(unsigned long long table);

__attribute__((always_inline)) extern unsigned long long
hvx_vlut_64u64(unsigned char indices, unsigned long long table);

__attribute__((always_inline)) extern unsigned long long
hvx_prepare_lut_128u64(unsigned long long table);

__attribute__((always_inline)) extern unsigned long long
hvx_vlut_128u64(unsigned char indices, unsigned long long table);

// f16 lookups.  Bit-identical to the u16 lookups (the lookup only moves whole
// elements); implemented in the runtime by reinterpreting _Float16<->u16.
#if __has_Float16__
__attribute__((always_inline)) extern _Float16
hvx_prepare_lut_128f16(_Float16 table);

__attribute__((always_inline)) extern _Float16
hvx_vlut_128f16(unsigned char indices, _Float16 table);

__attribute__((always_inline)) extern _Float16
hvx_prepare_lut_256f16(_Float16 table);

__attribute__((always_inline)) extern _Float16
hvx_vlut_256f16(unsigned char indices, _Float16 table);
#endif // __has_Float16__

// bf16 lookups.  Bit-identical to the u16 lookups; implemented in the runtime
// by reinterpreting __bf16<->u16.
#if __has_bf16__
__attribute__((always_inline)) extern __bf16
hvx_prepare_lut_128bf16(__bf16 table);

__attribute__((always_inline)) extern __bf16
hvx_vlut_128bf16(unsigned char indices, __bf16 table);

__attribute__((always_inline)) extern __bf16
hvx_prepare_lut_256bf16(__bf16 table);

__attribute__((always_inline)) extern __bf16
hvx_vlut_256bf16(unsigned char indices, __bf16 table);
#endif // __has_bf16__

// f32 lookups.  Bit-identical to the u32 lookups (the lookup only moves whole
// elements); implemented in the runtime by reinterpreting float<->u32.
__attribute__((always_inline)) extern float hvx_prepare_lut_128f32(float table);

__attribute__((always_inline)) extern float
hvx_vlut_128f32(unsigned char indices, float table);

__attribute__((always_inline)) extern float hvx_prepare_lut_256f32(float table);

__attribute__((always_inline)) extern float
hvx_vlut_256f32(unsigned char indices, float table);

// f64 lookups.  Bit-identical to the u64 lookups; implemented by reinterpreting
// double<->u64.
__attribute__((always_inline)) extern double
hvx_prepare_lut_64f64(double table);

__attribute__((always_inline)) extern double
hvx_vlut_64f64(unsigned char indices, double table);

__attribute__((always_inline)) extern double
hvx_prepare_lut_128f64(double table);

__attribute__((always_inline)) extern double
hvx_vlut_128f64(unsigned char indices, double table);

#ifdef __cplusplus
} // extern "C"
#endif

// ripple_vector_candidate definitions.
// 256-entry 8-bit lookup table prepare (single 256-element load).
RIPPLE_CANDIDATE_INLINE v256u8 ripple_hvx_prepare_lut_256u8(v256u8 table) {
  v128u8 table_lo = Q6_V_lo_W(table);
  v128u8 table_hi = Q6_V_hi_W(table);
  v256u8 tmp_lo = Q6_W_vshuff_VVR(table_lo, table_lo, 64);
  v256u8 prepared_pair_lo =
      Q6_W_vshuff_VVR(Q6_V_hi_W(tmp_lo), Q6_V_lo_W(tmp_lo), 127);
  v128u8 prepared_lo = Q6_V_lo_W(prepared_pair_lo);
  v256u8 tmp_hi = Q6_W_vshuff_VVR(table_hi, table_hi, 64);
  v256u8 prepared_pair_hi =
      Q6_W_vshuff_VVR(Q6_V_hi_W(tmp_hi), Q6_V_lo_W(tmp_hi), 127);
  v128u8 prepared_hi = Q6_V_lo_W(prepared_pair_hi);
  return (v256u8)Q6_W_vcombine_VV(prepared_hi, prepared_lo);
}

// 256-entry 8-bit lookup table prepare (128x2 SPMD load).
RIPPLE_CANDIDATE_INLINE v256u8
ripple_arg0_t128x2u8_ret_t128x2u8_hvx_prepare_lut_128x2u8(v256u8 table) {
  v128u8 table_lo = Q6_V_lo_W(table);
  v128u8 table_hi = Q6_V_hi_W(table);
  v256u8 tmp_lo = Q6_W_vshuff_VVR(table_lo, table_lo, 64);
  v256u8 prepared_pair_lo =
      Q6_W_vshuff_VVR(Q6_V_hi_W(tmp_lo), Q6_V_lo_W(tmp_lo), 127);
  v128u8 prepared_lo = Q6_V_lo_W(prepared_pair_lo);
  v256u8 tmp_hi = Q6_W_vshuff_VVR(table_hi, table_hi, 64);
  v256u8 prepared_pair_hi =
      Q6_W_vshuff_VVR(Q6_V_hi_W(tmp_hi), Q6_V_lo_W(tmp_hi), 127);
  v128u8 prepared_hi = Q6_V_lo_W(prepared_pair_hi);
  return (v256u8)Q6_W_vcombine_VV(prepared_hi, prepared_lo);
}

// 128-entry 8-bit lookup table prepare (one 128-element half).
RIPPLE_CANDIDATE_INLINE v128u8 ripple_hvx_prepare_lut_128u8(v128u8 table) {
  v256u8 tmp = Q6_W_vshuff_VVR(table, table, 64);
  v256u8 prepared_pair = Q6_W_vshuff_VVR(Q6_V_hi_W(tmp), Q6_V_lo_W(tmp), 127);
  return Q6_V_lo_W(prepared_pair);
}

// 256-entry 8-bit lookup; prepared table supplied via a stack pointer
// (table_lo for entries 0-127, table_hi for entries 128-255).  Uses vlut32.
RIPPLE_CANDIDATE_INLINE v128u8
ripple_ew_pure_hvx_vlut_u8_w_ptr(v128u8 indices, const unsigned char *table) {
  const v256u8 *prepared = (const v256u8 *)table;
  v128u8 prepared_lo = Q6_V_lo_W(*prepared);
  v128u8 prepared_hi = Q6_V_hi_W(*prepared);
  v128i8 result = Q6_Vb_vlut32_VbVbR(indices, prepared_lo, 0);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 1);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 2);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 3);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 4);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 5);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 6);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 7);
  return result;
}

// 256-entry 8-bit lookup; prepared table supplied as an SPMD value.  Uses
// vlut32.
RIPPLE_CANDIDATE_INLINE v128u8
ripple_pure_arg0_t128u8_arg1_t128x2u8_hvx_vlut_u8_w_value(v128u8 indices,
                                                          v256u8 table) {
  v128u8 prepared_lo = Q6_V_lo_W(table);
  v128u8 prepared_hi = Q6_V_hi_W(table);
  v128i8 result = Q6_Vb_vlut32_VbVbR(indices, prepared_lo, 0);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 1);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 2);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_lo, 3);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 4);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 5);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 6);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, prepared_hi, 7);
  return result;
}

// 256-entry 8-bit lookup; both prepared halves supplied as SPMD values.  Uses
// vlut32.
RIPPLE_CANDIDATE_INLINE v128u8 ripple_ew_pure_hvx_vlut_u8_2vec(
    v128u8 indices, v128u8 table_lo, v128u8 table_hi) {
  v128i8 result = Q6_Vb_vlut32_VbVbR(indices, table_lo, 0);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_lo, 1);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_lo, 2);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_lo, 3);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_hi, 4);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_hi, 5);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_hi, 6);
  result = Q6_Vb_vlut32or_VbVbVbR(result, indices, table_hi, 7);
  return result;
}

// 64-entry 16-bit lookup table prepare.
RIPPLE_CANDIDATE_INLINE v64u16
ripple_arg0_t64u16_ret_t64u16_hvx_prepare_lut_64u16(v64u16 table) {
  v128u16 tmp = Q6_W_vshuff_VVR(table, table, 64);
  v128u16 prepared_pair = Q6_W_vshuff_VVR(Q6_V_hi_W(tmp), Q6_V_lo_W(tmp), -2);
  return Q6_V_lo_W(prepared_pair);
}

// 64-entry 16-bit lookup.  Uses vlut16.
RIPPLE_CANDIDATE_INLINE v128u16
ripple_pure_arg0_t64x2u8_arg1_t64u16_ret_t64x2u16_hvx_vlut_64u16(
    v128u8 indices, v64u16 prepared) {
  v128u16 result = Q6_Wh_vlut16_VbVhR(indices, prepared, 0);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 1);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 2);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, prepared, 3);
  return Q6_W_vshuff_VVR(Q6_V_hi_W(result), Q6_V_lo_W(result), -2);
}

// 128-entry 16-bit lookup table prepare.
RIPPLE_CANDIDATE_INLINE v128u16
ripple_arg0_t128u16_ret_t128u16_hvx_prepare_lut_128u16(v128u16 table) {
  v64u16 *in_ptr = (v64u16 *)&table;
  v128u16 prepared;
  v64u16 *out_ptr = (v64u16 *)&prepared;
  for (int i = 0; i < 2; i++) {
    v128u16 tmp = Q6_W_vshuff_VVR(*in_ptr, *in_ptr, 64);
    v128u16 prepared_pair = Q6_W_vshuff_VVR(Q6_V_hi_W(tmp), Q6_V_lo_W(tmp), -2);
    *out_ptr = Q6_V_lo_W(prepared_pair);
    in_ptr++;
    out_ptr++;
  }
  return prepared;
}

// 128-entry 16-bit lookup.  Uses vlut16.
RIPPLE_CANDIDATE_INLINE v128u16
ripple_pure_arg0_t128u8_arg1_t128u16_ret_t128u16_hvx_vlut_128u16(
    v128u8 indices, v128u16 prepared) {
  v64u16 *in_ptr = (v64u16 *)&prepared;
  v128u16 result = Q6_Wh_vlut16_VbVhR(indices, *in_ptr, 0);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 1);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 2);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 3);
  in_ptr++;
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 4);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 5);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 6);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 7);
  return Q6_W_vshuff_VVR(Q6_V_hi_W(result), Q6_V_lo_W(result), -2);
}

// 256-entry 16-bit lookup table prepare.
RIPPLE_CANDIDATE_INLINE v256u16
ripple_arg0_t128x2u16_ret_t128x2u16_hvx_prepare_lut_256u16(v256u16 table) {
  v64u16 *in_ptr = (v64u16 *)&table;
  v256u16 prepared;
  v64u16 *out_ptr = (v64u16 *)&prepared;
  for (int i = 0; i < 4; i++) {
    v128u16 tmp = Q6_W_vshuff_VVR(*in_ptr, *in_ptr, 64);
    v128u16 prepared_pair = Q6_W_vshuff_VVR(Q6_V_hi_W(tmp), Q6_V_lo_W(tmp), -2);
    *out_ptr = Q6_V_lo_W(prepared_pair);
    in_ptr++;
    out_ptr++;
  }
  return prepared;
}

// 256-entry 16-bit lookup.  Uses vlut16.
RIPPLE_CANDIDATE_INLINE v128u16
ripple_pure_arg0_t128u8_arg1_t128x2u16_ret_t128u16_hvx_vlut_256u16(
    v128u8 indices, v256u16 prepared) {
  v64u16 *in_ptr = (v64u16 *)&prepared;
  v128u16 result = Q6_Wh_vlut16_VbVhR(indices, *in_ptr, 0);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 1);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 2);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 3);
  in_ptr++;
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 4);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 5);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 6);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 7);
  in_ptr++;
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 8);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 9);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 10);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 11);
  in_ptr++;
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 12);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 13);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 14);
  result = Q6_Wh_vlut16or_WhVbVhR(result, indices, *in_ptr, 15);
  return Q6_W_vshuff_VVR(Q6_V_hi_W(result), Q6_V_lo_W(result), -2);
}

// 128-entry 32-bit lookup table prepare (via vlut16: lo/hi hword sub-tables).
RIPPLE_CANDIDATE_INLINE v128u32
ripple_arg0_t128u32_ret_t128u32_hvx_prepare_lut_128u32(v128u32 table) {
  HVX_Vector *in_ptr = (HVX_Vector *)&table;
  HVX_Vector V0 = in_ptr[0];
  HVX_Vector V1 = in_ptr[1];
  HVX_Vector V2 = in_ptr[2];
  HVX_Vector V3 = in_ptr[3];

  v128u32 prepared;
  HVX_Vector *out_ptr = (HVX_Vector *)&prepared;
  out_ptr[0] = Q6_Vh_vshuffe_VhVh(V1, V0);
  out_ptr[1] = Q6_Vh_vshuffe_VhVh(V3, V2);
  out_ptr[2] = Q6_Vh_vshuffo_VhVh(V1, V0);
  out_ptr[3] = Q6_Vh_vshuffo_VhVh(V3, V2);
  return prepared;
}

// 128-entry 32-bit lookup (via vlut16).
RIPPLE_CANDIDATE_INLINE v128u32
ripple_pure_arg0_t128u8_arg1_t128u32_ret_t128u32_hvx_vlut_128u32(
    v128u8 indices, v128u32 prepared) {
  HVX_Vector *prep = (HVX_Vector *)&prepared;

  v128u16 lo_res = Q6_Wh_vlut16_VbVhR(indices, prep[0], 0);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 1);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 2);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 3);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 4);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 5);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 6);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 7);

  v128u16 hi_res = Q6_Wh_vlut16_VbVhR(indices, prep[2], 0);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[2], 1);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[2], 2);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[2], 3);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[3], 4);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[3], 5);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[3], 6);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[3], 7);

  v128u16 lo_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(lo_res), Q6_V_lo_W(lo_res), -2);
  v128u16 hi_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(hi_res), Q6_V_lo_W(hi_res), -2);

  v128u32 result;
  v64u32 *out = (v64u32 *)&result;
  out[0] = (v64u32)Q6_W_vshuff_VVR(Q6_V_lo_W(hi_seq), Q6_V_lo_W(lo_seq), -2);
  out[1] = (v64u32)Q6_W_vshuff_VVR(Q6_V_hi_W(hi_seq), Q6_V_hi_W(lo_seq), -2);
  return result;
}

// 256-entry 32-bit lookup table prepare (via vlut16: lo/hi hword sub-tables).
RIPPLE_CANDIDATE_INLINE v256u32
ripple_arg0_t128x2u32_ret_t128x2u32_hvx_prepare_lut_256u32(v256u32 table) {
  HVX_Vector *in_ptr = (HVX_Vector *)&table;
  HVX_Vector V0 = in_ptr[0];
  HVX_Vector V1 = in_ptr[1];
  HVX_Vector V2 = in_ptr[2];
  HVX_Vector V3 = in_ptr[3];
  HVX_Vector V4 = in_ptr[4];
  HVX_Vector V5 = in_ptr[5];
  HVX_Vector V6 = in_ptr[6];
  HVX_Vector V7 = in_ptr[7];

  v256u32 prepared;
  HVX_Vector *out_ptr = (HVX_Vector *)&prepared;
  out_ptr[0] = Q6_Vh_vshuffe_VhVh(V1, V0);
  out_ptr[1] = Q6_Vh_vshuffe_VhVh(V3, V2);
  out_ptr[2] = Q6_Vh_vshuffe_VhVh(V5, V4);
  out_ptr[3] = Q6_Vh_vshuffe_VhVh(V7, V6);
  out_ptr[4] = Q6_Vh_vshuffo_VhVh(V1, V0);
  out_ptr[5] = Q6_Vh_vshuffo_VhVh(V3, V2);
  out_ptr[6] = Q6_Vh_vshuffo_VhVh(V5, V4);
  out_ptr[7] = Q6_Vh_vshuffo_VhVh(V7, V6);
  return prepared;
}

// 256-entry 32-bit lookup (via vlut16).
RIPPLE_CANDIDATE_INLINE v128u32
ripple_pure_arg0_t128u8_arg1_t128x2u32_ret_t128u32_hvx_vlut_256u32(
    v128u8 indices, v256u32 prepared) {
  HVX_Vector *prep = (HVX_Vector *)&prepared;

  v128u16 lo_res = Q6_Wh_vlut16_VbVhR(indices, prep[0], 0);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 1);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 2);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[0], 3);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 4);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 5);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 6);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[1], 7);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[2], 8);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[2], 9);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[2], 10);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[2], 11);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[3], 12);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[3], 13);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[3], 14);
  lo_res = Q6_Wh_vlut16or_WhVbVhR(lo_res, indices, prep[3], 15);

  v128u16 hi_res = Q6_Wh_vlut16_VbVhR(indices, prep[4], 0);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[4], 1);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[4], 2);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[4], 3);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[5], 4);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[5], 5);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[5], 6);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[5], 7);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[6], 8);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[6], 9);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[6], 10);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[6], 11);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[7], 12);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[7], 13);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[7], 14);
  hi_res = Q6_Wh_vlut16or_WhVbVhR(hi_res, indices, prep[7], 15);

  v128u16 lo_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(lo_res), Q6_V_lo_W(lo_res), -2);
  v128u16 hi_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(hi_res), Q6_V_lo_W(hi_res), -2);

  v128u32 result;
  v64u32 *out = (v64u32 *)&result;
  out[0] = (v64u32)Q6_W_vshuff_VVR(Q6_V_lo_W(hi_seq), Q6_V_lo_W(lo_seq), -2);
  out[1] = (v64u32)Q6_W_vshuff_VVR(Q6_V_hi_W(hi_seq), Q6_V_hi_W(lo_seq), -2);
  return result;
}

// 64-entry 64-bit lookup table prepare (via vlut16: four hword sub-tables).
RIPPLE_CANDIDATE_INLINE v64u64
ripple_arg0_t64u64_ret_t64u64_hvx_prepare_lut_64u64(v64u64 table) {
  HVX_Vector *in_ptr = (HVX_Vector *)&table;
  HVX_Vector V0 = in_ptr[0];
  HVX_Vector V1 = in_ptr[1];
  HVX_Vector V2 = in_ptr[2];
  HVX_Vector V3 = in_ptr[3];

  HVX_VectorPair P01 = Q6_W_vshuff_VVR(V2, V0, 4);
  HVX_VectorPair P23 = Q6_W_vshuff_VVR(V3, V1, 4);
  HVX_Vector lo01 = Q6_V_lo_W(P01);
  HVX_Vector hi01 = Q6_V_hi_W(P01);
  HVX_Vector lo23 = Q6_V_lo_W(P23);
  HVX_Vector hi23 = Q6_V_hi_W(P23);

  v64u64 prepared;
  HVX_Vector *out_ptr = (HVX_Vector *)&prepared;
  out_ptr[0] = Q6_Vh_vpacke_VwVw(lo23, lo01);
  out_ptr[1] = Q6_Vh_vpacko_VwVw(lo23, lo01);
  out_ptr[2] = Q6_Vh_vpacke_VwVw(hi23, hi01);
  out_ptr[3] = Q6_Vh_vpacko_VwVw(hi23, hi01);
  return prepared;
}

// 64-entry 64-bit lookup (via vlut16).
RIPPLE_CANDIDATE_INLINE v128u64
ripple_pure_arg0_t64x2u8_arg1_t64u64_ret_t64x2u64_hvx_vlut_64u64(
    v128u8 indices, v64u64 prepared) {
  HVX_Vector *prep = (HVX_Vector *)&prepared;

  v128u16 h0_res = Q6_Wh_vlut16_VbVhR(indices, prep[0], 0);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 1);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 2);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 3);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 4);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 5);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 6);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 7);

  v128u16 h1_res = Q6_Wh_vlut16_VbVhR(indices, prep[1], 0);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 1);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 2);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 3);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 4);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 5);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 6);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[1], 7);

  v128u16 h2_res = Q6_Wh_vlut16_VbVhR(indices, prep[2], 0);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 1);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 2);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 3);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 4);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 5);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 6);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[2], 7);

  v128u16 h3_res = Q6_Wh_vlut16_VbVhR(indices, prep[3], 0);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 1);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 2);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 3);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 4);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 5);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 6);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[3], 7);

  v128u16 h0_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h0_res), Q6_V_lo_W(h0_res), -2);
  v128u16 h1_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h1_res), Q6_V_lo_W(h1_res), -2);
  v128u16 h2_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h2_res), Q6_V_lo_W(h2_res), -2);
  v128u16 h3_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h3_res), Q6_V_lo_W(h3_res), -2);

  unsigned short *H0 = (unsigned short *)&h0_seq;
  unsigned short *H1 = (unsigned short *)&h1_seq;
  unsigned short *H2 = (unsigned short *)&h2_seq;
  unsigned short *H3 = (unsigned short *)&h3_seq;
  v128u64 result;
  unsigned long long *D = (unsigned long long *)&result;
  for (int i = 0; i < 128; ++i)
    D[i] = (unsigned long long)H0[i] | ((unsigned long long)H1[i] << 16) |
           ((unsigned long long)H2[i] << 32) |
           ((unsigned long long)H3[i] << 48);
  return result;
}

// 128-entry 64-bit lookup table prepare (via vlut16: four hword sub-table
// pairs, processed as two 64-entry halves).
RIPPLE_CANDIDATE_INLINE v128u64
ripple_arg0_t128u64_ret_t128u64_hvx_prepare_lut_128u64(v128u64 table) {
  HVX_Vector *in_ptr = (HVX_Vector *)&table;
  v128u64 result;
  HVX_Vector *res_ptr = (HVX_Vector *)&result;

  for (int half = 0; half < 2; half++) {
    HVX_Vector V0 = in_ptr[half * 4 + 0];
    HVX_Vector V1 = in_ptr[half * 4 + 1];
    HVX_Vector V2 = in_ptr[half * 4 + 2];
    HVX_Vector V3 = in_ptr[half * 4 + 3];

    HVX_VectorPair P01 = Q6_W_vshuff_VVR(V2, V0, 4);
    HVX_VectorPair P23 = Q6_W_vshuff_VVR(V3, V1, 4);
    HVX_Vector lo01 = Q6_V_lo_W(P01);
    HVX_Vector hi01 = Q6_V_hi_W(P01);
    HVX_Vector lo23 = Q6_V_lo_W(P23);
    HVX_Vector hi23 = Q6_V_hi_W(P23);

    res_ptr[0 * 2 + half] = Q6_Vh_vpacke_VwVw(lo23, lo01);
    res_ptr[1 * 2 + half] = Q6_Vh_vpacko_VwVw(lo23, lo01);
    res_ptr[2 * 2 + half] = Q6_Vh_vpacke_VwVw(hi23, hi01);
    res_ptr[3 * 2 + half] = Q6_Vh_vpacko_VwVw(hi23, hi01);
  }

  return result;
}

// 128-entry 64-bit lookup (via vlut16).
RIPPLE_CANDIDATE_INLINE v128u64
ripple_pure_arg0_t128u8_arg1_t128u64_ret_t128u64_hvx_vlut_128u64(
    v128u8 indices, v128u64 prepared) {
  HVX_Vector *prep = (HVX_Vector *)&prepared;

  v128u16 h0_res = Q6_Wh_vlut16_VbVhR(indices, prep[0], 0);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 1);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 2);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[0], 3);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[1], 4);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[1], 5);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[1], 6);
  h0_res = Q6_Wh_vlut16or_WhVbVhR(h0_res, indices, prep[1], 7);

  v128u16 h1_res = Q6_Wh_vlut16_VbVhR(indices, prep[2], 0);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[2], 1);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[2], 2);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[2], 3);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[3], 4);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[3], 5);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[3], 6);
  h1_res = Q6_Wh_vlut16or_WhVbVhR(h1_res, indices, prep[3], 7);

  v128u16 h2_res = Q6_Wh_vlut16_VbVhR(indices, prep[4], 0);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[4], 1);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[4], 2);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[4], 3);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[5], 4);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[5], 5);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[5], 6);
  h2_res = Q6_Wh_vlut16or_WhVbVhR(h2_res, indices, prep[5], 7);

  v128u16 h3_res = Q6_Wh_vlut16_VbVhR(indices, prep[6], 0);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[6], 1);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[6], 2);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[6], 3);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[7], 4);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[7], 5);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[7], 6);
  h3_res = Q6_Wh_vlut16or_WhVbVhR(h3_res, indices, prep[7], 7);

  v128u16 h0_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h0_res), Q6_V_lo_W(h0_res), -2);
  v128u16 h1_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h1_res), Q6_V_lo_W(h1_res), -2);
  v128u16 h2_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h2_res), Q6_V_lo_W(h2_res), -2);
  v128u16 h3_seq = Q6_W_vshuff_VVR(Q6_V_hi_W(h3_res), Q6_V_lo_W(h3_res), -2);

  unsigned short *H0 = (unsigned short *)&h0_seq;
  unsigned short *H1 = (unsigned short *)&h1_seq;
  unsigned short *H2 = (unsigned short *)&h2_seq;
  unsigned short *H3 = (unsigned short *)&h3_seq;
  v128u64 result;
  unsigned long long *D = (unsigned long long *)&result;
  for (int i = 0; i < 128; ++i)
    D[i] = (unsigned long long)H0[i] | ((unsigned long long)H1[i] << 16) |
           ((unsigned long long)H2[i] << 32) |
           ((unsigned long long)H3[i] << 48);
  return result;
}

// ===========================================================================
// Floating-point entry points.
//
// Floating-point lookups are bit-identical to the same-width unsigned-integer
// lookups: the lookup only moves whole elements around, it never interprets
// their numeric value.  Each float/double entry therefore reinterprets the
// vector as the same-width unsigned-integer vector (via __builtin_memcpy, which
// does not violate aliasing rules), delegates to the corresponding u32/u64
// implementation, and reinterprets the result back.  float maps to u32, double
// maps to u64.
// ===========================================================================

// 128-entry 32-bit float lookup table prepare (via u32).
RIPPLE_CANDIDATE_INLINE v128f32
ripple_arg0_t128f32_ret_t128f32_hvx_prepare_lut_128f32(v128f32 table) {
  v128u32 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128u32_ret_t128u32_hvx_prepare_lut_128u32(u);
  v128f32 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 128-entry 32-bit float lookup (via u32).
RIPPLE_CANDIDATE_INLINE v128f32
ripple_pure_arg0_t128u8_arg1_t128f32_ret_t128f32_hvx_vlut_128f32(
    v128u8 indices, v128f32 prepared) {
  v128u32 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  u = ripple_pure_arg0_t128u8_arg1_t128u32_ret_t128u32_hvx_vlut_128u32(indices,
                                                                       u);
  v128f32 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 32-bit float lookup table prepare (via u32).
RIPPLE_CANDIDATE_INLINE v256f32
ripple_arg0_t128x2f32_ret_t128x2f32_hvx_prepare_lut_256f32(v256f32 table) {
  v256u32 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128x2u32_ret_t128x2u32_hvx_prepare_lut_256u32(u);
  v256f32 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 32-bit float lookup (via u32).
RIPPLE_CANDIDATE_INLINE v128f32
ripple_pure_arg0_t128u8_arg1_t128x2f32_ret_t128f32_hvx_vlut_256f32(
    v128u8 indices, v256f32 prepared) {
  v256u32 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  v128u32 res =
      ripple_pure_arg0_t128u8_arg1_t128x2u32_ret_t128u32_hvx_vlut_256u32(
          indices, u);
  v128f32 r;
  __builtin_memcpy(&r, &res, sizeof(r));
  return r;
}

// 64-entry 64-bit double lookup table prepare (via u64).
RIPPLE_CANDIDATE_INLINE v64f64
ripple_arg0_t64f64_ret_t64f64_hvx_prepare_lut_64f64(v64f64 table) {
  v64u64 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t64u64_ret_t64u64_hvx_prepare_lut_64u64(u);
  v64f64 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 64-entry 64-bit double lookup (via u64).
RIPPLE_CANDIDATE_INLINE v128f64
ripple_pure_arg0_t64x2u8_arg1_t64f64_ret_t64x2f64_hvx_vlut_64f64(
    v128u8 indices, v64f64 prepared) {
  v64u64 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  v128u64 res =
      ripple_pure_arg0_t64x2u8_arg1_t64u64_ret_t64x2u64_hvx_vlut_64u64(indices,
                                                                       u);
  v128f64 r;
  __builtin_memcpy(&r, &res, sizeof(r));
  return r;
}

// 128-entry 64-bit double lookup table prepare (via u64).
RIPPLE_CANDIDATE_INLINE v128f64
ripple_arg0_t128f64_ret_t128f64_hvx_prepare_lut_128f64(v128f64 table) {
  v128u64 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128u64_ret_t128u64_hvx_prepare_lut_128u64(u);
  v128f64 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 128-entry 64-bit double lookup (via u64).
RIPPLE_CANDIDATE_INLINE v128f64
ripple_pure_arg0_t128u8_arg1_t128f64_ret_t128f64_hvx_vlut_128f64(
    v128u8 indices, v128f64 prepared) {
  v128u64 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  u = ripple_pure_arg0_t128u8_arg1_t128u64_ret_t128u64_hvx_vlut_128u64(indices,
                                                                       u);
  v128f64 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// ===========================================================================
// 16-bit floating-point entry points (_Float16 / __bf16).
//
// These are bit-identical to the 16-bit unsigned-integer (u16) lookups: the
// lookup only moves whole elements, it never interprets their numeric value.
// Each entry therefore reinterprets the vector as the same-width u16 vector
// (via __builtin_memcpy, which does not violate aliasing rules), delegates to
// the corresponding u16 implementation, and reinterprets the result back.
// _Float16 and __bf16 both map to u16.
// ===========================================================================

#if __has_Float16__
// 128-entry 16-bit _Float16 lookup table prepare (via u16).
RIPPLE_CANDIDATE_INLINE v128f16
ripple_arg0_t128f16_ret_t128f16_hvx_prepare_lut_128f16(v128f16 table) {
  v128u16 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128u16_ret_t128u16_hvx_prepare_lut_128u16(u);
  v128f16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 128-entry 16-bit _Float16 lookup (via u16).
RIPPLE_CANDIDATE_INLINE v128f16
ripple_pure_arg0_t128u8_arg1_t128f16_ret_t128f16_hvx_vlut_128f16(
    v128u8 indices, v128f16 prepared) {
  v128u16 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  u = ripple_pure_arg0_t128u8_arg1_t128u16_ret_t128u16_hvx_vlut_128u16(indices,
                                                                       u);
  v128f16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 16-bit _Float16 lookup table prepare (via u16).
RIPPLE_CANDIDATE_INLINE v256f16
ripple_arg0_t128x2f16_ret_t128x2f16_hvx_prepare_lut_256f16(v256f16 table) {
  v256u16 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128x2u16_ret_t128x2u16_hvx_prepare_lut_256u16(u);
  v256f16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 16-bit _Float16 lookup (via u16).
RIPPLE_CANDIDATE_INLINE v128f16
ripple_pure_arg0_t128u8_arg1_t128x2f16_ret_t128f16_hvx_vlut_256f16(
    v128u8 indices, v256f16 prepared) {
  v256u16 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  v128u16 res =
      ripple_pure_arg0_t128u8_arg1_t128x2u16_ret_t128u16_hvx_vlut_256u16(
          indices, u);
  v128f16 r;
  __builtin_memcpy(&r, &res, sizeof(r));
  return r;
}
#endif // __has_Float16__

#if __has_bf16__
// 128-entry 16-bit __bf16 lookup table prepare (via u16).
RIPPLE_CANDIDATE_INLINE v128bf16
ripple_arg0_t128bf16_ret_t128bf16_hvx_prepare_lut_128bf16(v128bf16 table) {
  v128u16 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128u16_ret_t128u16_hvx_prepare_lut_128u16(u);
  v128bf16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 128-entry 16-bit __bf16 lookup (via u16).
RIPPLE_CANDIDATE_INLINE v128bf16
ripple_pure_arg0_t128u8_arg1_t128bf16_ret_t128bf16_hvx_vlut_128bf16(
    v128u8 indices, v128bf16 prepared) {
  v128u16 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  u = ripple_pure_arg0_t128u8_arg1_t128u16_ret_t128u16_hvx_vlut_128u16(indices,
                                                                       u);
  v128bf16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 16-bit __bf16 lookup table prepare (via u16).
RIPPLE_CANDIDATE_INLINE v256bf16
ripple_arg0_t128x2bf16_ret_t128x2bf16_hvx_prepare_lut_256bf16(v256bf16 table) {
  v256u16 u;
  __builtin_memcpy(&u, &table, sizeof(u));
  u = ripple_arg0_t128x2u16_ret_t128x2u16_hvx_prepare_lut_256u16(u);
  v256bf16 r;
  __builtin_memcpy(&r, &u, sizeof(r));
  return r;
}

// 256-entry 16-bit __bf16 lookup (via u16).
RIPPLE_CANDIDATE_INLINE v128bf16
ripple_pure_arg0_t128u8_arg1_t128x2bf16_ret_t128bf16_hvx_vlut_256bf16(
    v128u8 indices, v256bf16 prepared) {
  v256u16 u;
  __builtin_memcpy(&u, &prepared, sizeof(u));
  v128u16 res =
      ripple_pure_arg0_t128u8_arg1_t128x2u16_ret_t128u16_hvx_vlut_256u16(
          indices, u);
  v128bf16 r;
  __builtin_memcpy(&r, &res, sizeof(r));
  return r;
}
#endif // __has_bf16__

// =============================================================================
// Polymorphic wrappers
// =============================================================================

#ifndef __cplusplus

// --- C: _Generic dispatch ---

// _Float16 / __bf16 are only available when the target supports them, so the
// corresponding _Generic associations are emitted conditionally as macro
// fragments.
#if __has_Float16__
#define __HVX_VLUT_F16_PREPARE(table)                                          \
  , _Float16 : hvx_prepare_lut_128f16((table))
#define __HVX_VLUT_F16_VLUT(indices, prepared)                                 \
  , _Float16 : hvx_vlut_128f16((indices), (prepared))
#define __HVX_VLUT_F16_PREPARE_DOUBLE(table)                                   \
  , _Float16 : hvx_prepare_lut_256f16((table))
#define __HVX_VLUT_F16_VLUT_DOUBLE(indices, prepared)                          \
  , _Float16 : hvx_vlut_256f16((indices), (prepared))
#else
#define __HVX_VLUT_F16_PREPARE(table)
#define __HVX_VLUT_F16_VLUT(indices, prepared)
#define __HVX_VLUT_F16_PREPARE_DOUBLE(table)
#define __HVX_VLUT_F16_VLUT_DOUBLE(indices, prepared)
#endif

#if __has_bf16__
#define __HVX_VLUT_BF16_PREPARE(table)                                         \
  , __bf16 : hvx_prepare_lut_128bf16((table))
#define __HVX_VLUT_BF16_VLUT(indices, prepared)                                \
  , __bf16 : hvx_vlut_128bf16((indices), (prepared))
#define __HVX_VLUT_BF16_PREPARE_DOUBLE(table)                                  \
  , __bf16 : hvx_prepare_lut_256bf16((table))
#define __HVX_VLUT_BF16_VLUT_DOUBLE(indices, prepared)                         \
  , __bf16 : hvx_vlut_256bf16((indices), (prepared))
#else
#define __HVX_VLUT_BF16_PREPARE(table)
#define __HVX_VLUT_BF16_VLUT(indices, prepared)
#define __HVX_VLUT_BF16_PREPARE_DOUBLE(table)
#define __HVX_VLUT_BF16_VLUT_DOUBLE(indices, prepared)
#endif

// hvx_prepare_lut(table) — build the prepared lookup table; dispatches on the
// element type of the table entry.
#define hvx_prepare_lut(table)                                                 \
  _Generic((table),                                                            \
      unsigned short: hvx_prepare_lut_128u16((table)),                         \
      short: (short)hvx_prepare_lut_128u16((unsigned short)(table)),           \
      unsigned int: hvx_prepare_lut_128u32((table)),                           \
      int: (int)hvx_prepare_lut_128u32((unsigned int)(table)),                 \
      unsigned long: hvx_prepare_lut_128u32((table)),                          \
      long: (long)hvx_prepare_lut_128u32((unsigned long)(table)),              \
      unsigned long long: hvx_prepare_lut_64u64((table)),                      \
      long long: (long long)hvx_prepare_lut_64u64(                             \
               (unsigned long long)(table)),                                   \
      float: hvx_prepare_lut_128f32((table)),                                  \
      double: hvx_prepare_lut_64f64((table)) __HVX_VLUT_F16_PREPARE((table))   \
          __HVX_VLUT_BF16_PREPARE((table)))

// hvx_vlut(indices, prepared) — in-lane lookup; dispatches on the element type
// of the prepared table.
#define hvx_vlut(indices, prepared)                                            \
  _Generic((prepared),                                                         \
      unsigned short: hvx_vlut_128u16((indices), (prepared)),                  \
      short: (short)hvx_vlut_128u16((indices), (unsigned short)(prepared)),    \
      unsigned int: hvx_vlut_128u32((indices), (prepared)),                    \
      int: (int)hvx_vlut_128u32((indices), (unsigned int)(prepared)),          \
      unsigned long: hvx_vlut_128u32((indices), (prepared)),                   \
      long: (long)hvx_vlut_128u32((indices), (unsigned long)(prepared)),       \
      unsigned long long: hvx_vlut_64u64((indices), (prepared)),               \
      long long: (long long)hvx_vlut_64u64((indices),                          \
                                           (unsigned long long)(prepared)),    \
      float: hvx_vlut_128f32((indices), (prepared)),                           \
      double: hvx_vlut_64f64((indices), (prepared))                            \
          __HVX_VLUT_F16_VLUT((indices), (prepared))                           \
              __HVX_VLUT_BF16_VLUT((indices), (prepared)))

// hvx_prepare_lut_double(table) — build the double-length prepared table (twice
// the single-block capacity); dispatches on element type to the double-length
// variant for that type:
//   unsigned short              -> 256-entry u16
//   unsigned int / unsigned long -> 256-entry u32
//   unsigned long long          -> 128-entry u64
#define hvx_prepare_lut_double(table)                                          \
  _Generic((table),                                                            \
      unsigned short: hvx_prepare_lut_256u16((table)),                         \
      short: (short)hvx_prepare_lut_256u16((unsigned short)(table)),           \
      unsigned int: hvx_prepare_lut_256u32((table)),                           \
      int: (int)hvx_prepare_lut_256u32((unsigned int)(table)),                 \
      unsigned long: hvx_prepare_lut_256u32((table)),                          \
      long: (long)hvx_prepare_lut_256u32((unsigned long)(table)),              \
      unsigned long long: hvx_prepare_lut_128u64((table)),                     \
      long long: (long long)hvx_prepare_lut_128u64(                            \
               (unsigned long long)(table)),                                   \
      float: hvx_prepare_lut_256f32((table)),                                  \
      double: hvx_prepare_lut_128f64((table))                                  \
          __HVX_VLUT_F16_PREPARE_DOUBLE((table))                               \
              __HVX_VLUT_BF16_PREPARE_DOUBLE((table)))

// hvx_vlut_double(indices, prepared) — in-lane lookup over a
// double-vector-length prepared table; dispatches on the element type of the
// prepared table.
#define hvx_vlut_double(indices, prepared)                                     \
  _Generic((prepared),                                                         \
      unsigned short: hvx_vlut_256u16((indices), (prepared)),                  \
      short: (short)hvx_vlut_256u16((indices), (unsigned short)(prepared)),    \
      unsigned int: hvx_vlut_256u32((indices), (prepared)),                    \
      int: (int)hvx_vlut_256u32((indices), (unsigned int)(prepared)),          \
      unsigned long: hvx_vlut_256u32((indices), (prepared)),                   \
      long: (long)hvx_vlut_256u32((indices), (unsigned long)(prepared)),       \
      unsigned long long: hvx_vlut_128u64((indices), (prepared)),              \
      long long: (long long)hvx_vlut_128u64((indices),                         \
                                            (unsigned long long)(prepared)),   \
      float: hvx_vlut_256f32((indices), (prepared)),                           \
      double: hvx_vlut_128f64((indices), (prepared))                           \
          __HVX_VLUT_F16_VLUT_DOUBLE((indices), (prepared))                    \
              __HVX_VLUT_BF16_VLUT_DOUBLE((indices), (prepared)))

#else // __cplusplus

// --- C++: overloaded static inline functions ---

// hvx_prepare_lut(table)
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_prepare_lut(unsigned short table) {
  return hvx_prepare_lut_128u16(table);
}
[[gnu::always_inline, gnu::unused]] static short hvx_prepare_lut(short table) {
  return (short)hvx_prepare_lut_128u16((unsigned short)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_prepare_lut(unsigned int table) {
  return hvx_prepare_lut_128u32(table);
}
[[gnu::always_inline, gnu::unused]] static int hvx_prepare_lut(int table) {
  return (int)hvx_prepare_lut_128u32((unsigned int)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_prepare_lut(unsigned long table) {
  return hvx_prepare_lut_128u32(table);
}
[[gnu::always_inline, gnu::unused]] static long hvx_prepare_lut(long table) {
  return (long)hvx_prepare_lut_128u32((unsigned long)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned long long
hvx_prepare_lut(unsigned long long table) {
  return hvx_prepare_lut_64u64(table);
}
[[gnu::always_inline, gnu::unused]] static long long
hvx_prepare_lut(long long table) {
  return (long long)hvx_prepare_lut_64u64((unsigned long long)table);
}
[[gnu::always_inline, gnu::unused]] static float hvx_prepare_lut(float table) {
  return hvx_prepare_lut_128f32(table);
}
[[gnu::always_inline, gnu::unused]] static double
hvx_prepare_lut(double table) {
  return hvx_prepare_lut_64f64(table);
}
#if __has_Float16__
[[gnu::always_inline,
  gnu::unused]] static _Float16 hvx_prepare_lut(_Float16 table) {
  return hvx_prepare_lut_128f16(table);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline,
  gnu::unused]] static __bf16 hvx_prepare_lut(__bf16 table) {
  return hvx_prepare_lut_128bf16(table);
}
#endif // __has_bf16__

// hvx_vlut(indices, prepared)
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vlut(unsigned char indices, unsigned short prepared) {
  return hvx_vlut_128u16(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static short hvx_vlut(unsigned char indices,
                                                          short prepared) {
  return (short)hvx_vlut_128u16(indices, (unsigned short)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_vlut(unsigned char indices, unsigned int prepared) {
  return hvx_vlut_128u32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static int hvx_vlut(unsigned char indices,
                                                        int prepared) {
  return (int)hvx_vlut_128u32(indices, (unsigned int)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_vlut(unsigned char indices, unsigned long prepared) {
  return hvx_vlut_128u32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static long hvx_vlut(unsigned char indices,
                                                         long prepared) {
  return (long)hvx_vlut_128u32(indices, (unsigned long)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned long long
hvx_vlut(unsigned char indices, unsigned long long prepared) {
  return hvx_vlut_64u64(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static long long
hvx_vlut(unsigned char indices, long long prepared) {
  return (long long)hvx_vlut_64u64(indices, (unsigned long long)prepared);
}
[[gnu::always_inline, gnu::unused]] static float hvx_vlut(unsigned char indices,
                                                          float prepared) {
  return hvx_vlut_128f32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static double
hvx_vlut(unsigned char indices, double prepared) {
  return hvx_vlut_64f64(indices, prepared);
}
#if __has_Float16__
[[gnu::always_inline, gnu::unused]] static _Float16
hvx_vlut(unsigned char indices, _Float16 prepared) {
  return hvx_vlut_128f16(indices, prepared);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline,
  gnu::unused]] static __bf16 hvx_vlut(unsigned char indices, __bf16 prepared) {
  return hvx_vlut_128bf16(indices, prepared);
}
#endif // __has_bf16__

// hvx_prepare_lut_double(table) — double-vector-length (2-D) prepared table.
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_prepare_lut_double(unsigned short table) {
  return hvx_prepare_lut_256u16(table);
}
[[gnu::always_inline, gnu::unused]] static short
hvx_prepare_lut_double(short table) {
  return (short)hvx_prepare_lut_256u16((unsigned short)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_prepare_lut_double(unsigned int table) {
  return hvx_prepare_lut_256u32(table);
}
[[gnu::always_inline, gnu::unused]] static int
hvx_prepare_lut_double(int table) {
  return (int)hvx_prepare_lut_256u32((unsigned int)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_prepare_lut_double(unsigned long table) {
  return hvx_prepare_lut_256u32(table);
}
[[gnu::always_inline, gnu::unused]] static long
hvx_prepare_lut_double(long table) {
  return (long)hvx_prepare_lut_256u32((unsigned long)table);
}
[[gnu::always_inline, gnu::unused]] static unsigned long long
hvx_prepare_lut_double(unsigned long long table) {
  return hvx_prepare_lut_128u64(table);
}
[[gnu::always_inline, gnu::unused]] static long long
hvx_prepare_lut_double(long long table) {
  return (long long)hvx_prepare_lut_128u64((unsigned long long)table);
}
[[gnu::always_inline, gnu::unused]] static float
hvx_prepare_lut_double(float table) {
  return hvx_prepare_lut_256f32(table);
}
[[gnu::always_inline, gnu::unused]] static double
hvx_prepare_lut_double(double table) {
  return hvx_prepare_lut_128f64(table);
}
#if __has_Float16__
[[gnu::always_inline,
  gnu::unused]] static _Float16 hvx_prepare_lut_double(_Float16 table) {
  return hvx_prepare_lut_256f16(table);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline,
  gnu::unused]] static __bf16 hvx_prepare_lut_double(__bf16 table) {
  return hvx_prepare_lut_256bf16(table);
}
#endif // __has_bf16__

// hvx_vlut_double(indices, prepared) — lookup over a double-vector-length
// table.
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vlut_double(unsigned char indices, unsigned short prepared) {
  return hvx_vlut_256u16(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static short
hvx_vlut_double(unsigned char indices, short prepared) {
  return (short)hvx_vlut_256u16(indices, (unsigned short)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_vlut_double(unsigned char indices, unsigned int prepared) {
  return hvx_vlut_256u32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static int
hvx_vlut_double(unsigned char indices, int prepared) {
  return (int)hvx_vlut_256u32(indices, (unsigned int)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_vlut_double(unsigned char indices, unsigned long prepared) {
  return hvx_vlut_256u32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static long
hvx_vlut_double(unsigned char indices, long prepared) {
  return (long)hvx_vlut_256u32(indices, (unsigned long)prepared);
}
[[gnu::always_inline, gnu::unused]] static unsigned long long
hvx_vlut_double(unsigned char indices, unsigned long long prepared) {
  return hvx_vlut_128u64(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static long long
hvx_vlut_double(unsigned char indices, long long prepared) {
  return (long long)hvx_vlut_128u64(indices, (unsigned long long)prepared);
}
[[gnu::always_inline, gnu::unused]] static float
hvx_vlut_double(unsigned char indices, float prepared) {
  return hvx_vlut_256f32(indices, prepared);
}
[[gnu::always_inline, gnu::unused]] static double
hvx_vlut_double(unsigned char indices, double prepared) {
  return hvx_vlut_128f64(indices, prepared);
}
#if __has_Float16__
[[gnu::always_inline, gnu::unused]] static _Float16
hvx_vlut_double(unsigned char indices, _Float16 prepared) {
  return hvx_vlut_256f16(indices, prepared);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline, gnu::unused]] static __bf16
hvx_vlut_double(unsigned char indices, __bf16 prepared) {
  return hvx_vlut_256bf16(indices, prepared);
}
#endif // __has_bf16__

#endif // __cplusplus
