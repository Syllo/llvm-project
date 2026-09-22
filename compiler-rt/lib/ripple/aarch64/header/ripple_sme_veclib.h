//===-- ripple_sme_veclib.h - Ripple SME runtime helpers
//-------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Compile callers with SME enabled. The Ripple t32x32 interfaces use a
// 512-bit streaming vector length (-msve-streaming-vector-bits=512).
//
//===----------------------------------------------------------------------===//

#pragma once

#include <arm_sme.h>
#include <ripple.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

// Internal linkage permits inclusion in multiple C and C++ translation units.
// Do not force inlining across incompatible SME streaming interfaces.
#define RIPPLE_SME_CANDIDATE_INLINE                                            \
  static inline __attribute__((unused)) RIPPLE_VECTOR_CANDIDATE

#ifdef __cplusplus
extern "C" {
#endif

// WLANES represents number of 32-bit lanes (4 * vscale).
#define RIPPLE_API_FIXED_WLANES 16

// Verify if Ripple operation fixed vscale is same as runtime vscale.
static inline bool verify_ripple_sme_api_fixed_vscale(void) {
  return svcntw() == RIPPLE_API_FIXED_WLANES;
}

// Clear ZA and set up bias value.
RIPPLE_SME_CANDIDATE_INLINE void
ripple_fx_t32x32_sme_clearacc(void) __arm_streaming __arm_out("za") {
  svzero_za();
}

RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_clearacc_set_bias_i32(
    const int32_t *bias) __arm_streaming __arm_out("za") {
  svzero_za();

  svint32_t Bias0 = svld1_s32(svptrue_b32(), bias);
  svint32_t Bias1 = svld1_s32(svptrue_b32(), bias + svcntw());

  // Per-column bias, add to each horizontal tile slices of ZA tiles 0..3.
  svaddha_za32_s32_m(0, svptrue_b32(), svptrue_b32(), Bias0);
  svaddha_za32_s32_m(1, svptrue_b32(), svptrue_b32(), Bias1);
  svaddha_za32_s32_m(2, svptrue_b32(), svptrue_b32(), Bias0);
  svaddha_za32_s32_m(3, svptrue_b32(), svptrue_b32(), Bias1);
}

RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_clearacc_set_bias_f32(
    const float *bias) __arm_streaming __arm_out("za") {
  svzero_za();

  svfloat32_t bias0 = svld1_f32(svptrue_b32(), bias);
  svfloat32_t bias1 = svld1_f32(svptrue_b32(), bias + svcntw());

  // Broadcast 1.0f.
  svfloat32_t one = svdup_f32(1.0f);

  // Apply outer product to ZA tiles 0..3.
  svmopa_za32_m(0, svptrue_b32(), svptrue_b32(), one, bias0);
  svmopa_za32_m(1, svptrue_b32(), svptrue_b32(), one, bias1);
  svmopa_za32_m(2, svptrue_b32(), svptrue_b32(), one, bias0);
  svmopa_za32_m(3, svptrue_b32(), svptrue_b32(), one, bias1);
}

// Outer product for int8 path, K = 4.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_outeracc_k4_i32(
    const int8_t *A, const int8_t *B) __arm_streaming __arm_inout("za") {
  svint8_t A0 = svld1_s8(svptrue_b8(), A);
  svint8_t A1 = svld1_s8(svptrue_b8(), A + svcntb());
  svint8_t B0 = svld1_s8(svptrue_b8(), B);
  svint8_t B1 = svld1_s8(svptrue_b8(), B + svcntb());

  // Multiply and accumulate int8_t into ZA tiles (32-bit accumulators).
  svmopa_za32_m(0, svptrue_b8(), svptrue_b8(), A0, B0);
  svmopa_za32_m(1, svptrue_b8(), svptrue_b8(), A0, B1);
  svmopa_za32_m(2, svptrue_b8(), svptrue_b8(), A1, B0);
  svmopa_za32_m(3, svptrue_b8(), svptrue_b8(), A1, B1);
}

// Outer product for int8 path, K = 16 unrolled.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_outeracc_k16_unr_i32(
    const int8_t *A, const int8_t *B) __arm_streaming __arm_inout("za") {
  svint8_t A0 = svld1_s8(svptrue_b8(), A);
  svint8_t A1 = svld1_s8(svptrue_b8(), A + svcntb());
  svint8_t A2 = svld1_s8(svptrue_b8(), A + 2 * svcntb());
  svint8_t A3 = svld1_s8(svptrue_b8(), A + 3 * svcntb());
  svint8_t A4 = svld1_s8(svptrue_b8(), A + 4 * svcntb());
  svint8_t A5 = svld1_s8(svptrue_b8(), A + 5 * svcntb());
  svint8_t A6 = svld1_s8(svptrue_b8(), A + 6 * svcntb());
  svint8_t A7 = svld1_s8(svptrue_b8(), A + 7 * svcntb());
  svint8_t B0 = svld1_s8(svptrue_b8(), B);
  svint8_t B1 = svld1_s8(svptrue_b8(), B + svcntb());
  svint8_t B2 = svld1_s8(svptrue_b8(), B + 2 * svcntb());
  svint8_t B3 = svld1_s8(svptrue_b8(), B + 3 * svcntb());
  svint8_t B4 = svld1_s8(svptrue_b8(), B + 4 * svcntb());
  svint8_t B5 = svld1_s8(svptrue_b8(), B + 5 * svcntb());
  svint8_t B6 = svld1_s8(svptrue_b8(), B + 6 * svcntb());
  svint8_t B7 = svld1_s8(svptrue_b8(), B + 7 * svcntb());

  // Multiply and accumulate int8_t into ZA tile slices (32-bit accumulators).
  svmopa_za32_m(0, svptrue_b8(), svptrue_b8(), A0, B0);
  svmopa_za32_m(1, svptrue_b8(), svptrue_b8(), A0, B1);
  svmopa_za32_m(2, svptrue_b8(), svptrue_b8(), A1, B0);
  svmopa_za32_m(3, svptrue_b8(), svptrue_b8(), A1, B1);
  svmopa_za32_m(0, svptrue_b8(), svptrue_b8(), A2, B2);
  svmopa_za32_m(1, svptrue_b8(), svptrue_b8(), A2, B3);
  svmopa_za32_m(2, svptrue_b8(), svptrue_b8(), A3, B2);
  svmopa_za32_m(3, svptrue_b8(), svptrue_b8(), A3, B3);
  svmopa_za32_m(0, svptrue_b8(), svptrue_b8(), A4, B4);
  svmopa_za32_m(1, svptrue_b8(), svptrue_b8(), A4, B5);
  svmopa_za32_m(2, svptrue_b8(), svptrue_b8(), A5, B4);
  svmopa_za32_m(3, svptrue_b8(), svptrue_b8(), A5, B5);
  svmopa_za32_m(0, svptrue_b8(), svptrue_b8(), A6, B6);
  svmopa_za32_m(1, svptrue_b8(), svptrue_b8(), A6, B7);
  svmopa_za32_m(2, svptrue_b8(), svptrue_b8(), A7, B6);
  svmopa_za32_m(3, svptrue_b8(), svptrue_b8(), A7, B7);
}

// Outer product for fp32 path.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_outeracc_f32(
    const float *A, const float *B) __arm_streaming __arm_inout("za") {
  svfloat32_t A0 = svld1_f32(svptrue_b32(), A);
  svfloat32_t A1 = svld1_f32(svptrue_b32(), A + svcntw());
  svfloat32_t B0 = svld1_f32(svptrue_b32(), B);
  svfloat32_t B1 = svld1_f32(svptrue_b32(), B + svcntw());

  // Multiply and accumulate float into ZA tile slices (32-bit accumulators).
  svmopa_za32_m(0, svptrue_b32(), svptrue_b32(), A0, B0);
  svmopa_za32_m(1, svptrue_b32(), svptrue_b32(), A0, B1);
  svmopa_za32_m(2, svptrue_b32(), svptrue_b32(), A1, B0);
  svmopa_za32_m(3, svptrue_b32(), svptrue_b32(), A1, B1);
}

// Read int8 data(clamped, re-quantized) from ZA accumulator to memory.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_readacc_w_clamp_i8(
    int8_t *tile_base, const float *scale, size_t row_stride, int clamp_min,
    int clamp_max, int zero_point) __arm_streaming __arm_in("za") {
  // Loading scale into two scalable vectors.
  svfloat32_t scale0 = svld1_f32(svptrue_b32(), scale);
  svfloat32_t scale1 = svld1_f32(svptrue_b32(), scale + svcntw());
  svint32_t min = svdup_s32(clamp_min);
  svint32_t max = svdup_s32(clamp_max);
  svint32_t zp = svdup_s32(zero_point);
  enum { UnrollFactor = 4 };

  for (uint64_t slice = 0; slice < svcntw(); slice += UnrollFactor) {
    svint32_t t00 = svread_hor_za32_m(svundef_s32(), svptrue_b32(), 0, slice);
    svint32_t t10 = svread_hor_za32_m(svundef_s32(), svptrue_b32(), 1, slice);
    svint32_t t01 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 0, slice + 1);
    svint32_t t11 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 1, slice + 1);
    svint32_t t02 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 0, slice + 2);
    svint32_t t12 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 1, slice + 2);
    svint32_t t03 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 0, slice + 3);
    svint32_t t13 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 1, slice + 3);

    // scvtf (int32 -> fp32).
    svfloat32_t f00 = svcvt_f32_s32_x(svptrue_b32(), t00);
    svfloat32_t f10 = svcvt_f32_s32_x(svptrue_b32(), t10);
    svfloat32_t f01 = svcvt_f32_s32_x(svptrue_b32(), t01);
    svfloat32_t f11 = svcvt_f32_s32_x(svptrue_b32(), t11);
    svfloat32_t f02 = svcvt_f32_s32_x(svptrue_b32(), t02);
    svfloat32_t f12 = svcvt_f32_s32_x(svptrue_b32(), t12);
    svfloat32_t f03 = svcvt_f32_s32_x(svptrue_b32(), t03);
    svfloat32_t f13 = svcvt_f32_s32_x(svptrue_b32(), t13);

    // Apply per-column scale.
    svfloat32_t fmul00 = svmul_f32_x(svptrue_b32(), f00, scale0);
    svfloat32_t fmul10 = svmul_f32_x(svptrue_b32(), f10, scale1);
    svfloat32_t fmul01 = svmul_f32_x(svptrue_b32(), f01, scale0);
    svfloat32_t fmul11 = svmul_f32_x(svptrue_b32(), f11, scale1);
    svfloat32_t fmul02 = svmul_f32_x(svptrue_b32(), f02, scale0);
    svfloat32_t fmul12 = svmul_f32_x(svptrue_b32(), f12, scale1);
    svfloat32_t fmul03 = svmul_f32_x(svptrue_b32(), f03, scale0);
    svfloat32_t fmul13 = svmul_f32_x(svptrue_b32(), f13, scale1);

    // frintn (round to nearest int, ties to even), then fcvtzs (fp32->s32).
    svint32_t s00 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul00));
    svint32_t s10 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul10));
    svint32_t s01 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul01));
    svint32_t s11 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul11));
    svint32_t s02 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul02));
    svint32_t s12 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul12));
    svint32_t s03 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul03));
    svint32_t s13 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul13));

    // Add zero point.
    svint32_t zp00 = svadd_s32_x(svptrue_b32(), s00, zp);
    svint32_t zp10 = svadd_s32_x(svptrue_b32(), s10, zp);
    svint32_t zp01 = svadd_s32_x(svptrue_b32(), s01, zp);
    svint32_t zp11 = svadd_s32_x(svptrue_b32(), s11, zp);
    svint32_t zp02 = svadd_s32_x(svptrue_b32(), s02, zp);
    svint32_t zp12 = svadd_s32_x(svptrue_b32(), s12, zp);
    svint32_t zp03 = svadd_s32_x(svptrue_b32(), s03, zp);
    svint32_t zp13 = svadd_s32_x(svptrue_b32(), s13, zp);

    // Clamp.
    svint32_t v00 = svmin_s32_x(svptrue_b32(), zp00, max);
    v00 = svmax_s32_x(svptrue_b32(), v00, min);
    svint32_t v10 = svmin_s32_x(svptrue_b32(), zp10, max);
    v10 = svmax_s32_x(svptrue_b32(), v10, min);
    svint32_t v01 = svmin_s32_x(svptrue_b32(), zp01, max);
    v01 = svmax_s32_x(svptrue_b32(), v01, min);
    svint32_t v11 = svmin_s32_x(svptrue_b32(), zp11, max);
    v11 = svmax_s32_x(svptrue_b32(), v11, min);
    svint32_t v02 = svmin_s32_x(svptrue_b32(), zp02, max);
    v02 = svmax_s32_x(svptrue_b32(), v02, min);
    svint32_t v12 = svmin_s32_x(svptrue_b32(), zp12, max);
    v12 = svmax_s32_x(svptrue_b32(), v12, min);
    svint32_t v03 = svmin_s32_x(svptrue_b32(), zp03, max);
    v03 = svmax_s32_x(svptrue_b32(), v03, min);
    svint32_t v13 = svmin_s32_x(svptrue_b32(), zp13, max);
    v13 = svmax_s32_x(svptrue_b32(), v13, min);

    // Reinterpret s32 -> s16, then uzp1 to interleave low halfwords.
    svint16_t h0 =
        svuzp1_s16(svreinterpret_s16_s32(v00), svreinterpret_s16_s32(v10));
    svint16_t h1 =
        svuzp1_s16(svreinterpret_s16_s32(v01), svreinterpret_s16_s32(v11));
    svint16_t h2 =
        svuzp1_s16(svreinterpret_s16_s32(v02), svreinterpret_s16_s32(v12));
    svint16_t h3 =
        svuzp1_s16(svreinterpret_s16_s32(v03), svreinterpret_s16_s32(v13));

    int8_t *addr0 = tile_base + slice * row_stride;
    // Truncating stores of low 8 bits.
    svst1b_s16(svptrue_b8(), addr0, h0);
    svst1b_s16(svptrue_b8(), addr0 + row_stride, h1);
    svst1b_s16(svptrue_b8(), addr0 + 2 * row_stride, h2);
    svst1b_s16(svptrue_b8(), addr0 + 3 * row_stride, h3);
  }

  uint64_t tile2_offset = svcntw() * row_stride;
  for (uint64_t slice = 0; slice < svcntw(); slice += UnrollFactor) {
    svint32_t r20 = svread_hor_za32_m(svundef_s32(), svptrue_b32(), 2, slice);
    svint32_t r30 = svread_hor_za32_m(svundef_s32(), svptrue_b32(), 3, slice);
    svint32_t r21 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 2, slice + 1);
    svint32_t r31 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 3, slice + 1);
    svint32_t r22 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 2, slice + 2);
    svint32_t r32 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 3, slice + 2);
    svint32_t r23 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 2, slice + 3);
    svint32_t r33 =
        svread_hor_za32_m(svundef_s32(), svptrue_b32(), 3, slice + 3);

    // scvtf (int32 -> fp32).
    svfloat32_t f20 = svcvt_f32_s32_x(svptrue_b32(), r20);
    svfloat32_t f30 = svcvt_f32_s32_x(svptrue_b32(), r30);
    svfloat32_t f21 = svcvt_f32_s32_x(svptrue_b32(), r21);
    svfloat32_t f31 = svcvt_f32_s32_x(svptrue_b32(), r31);
    svfloat32_t f22 = svcvt_f32_s32_x(svptrue_b32(), r22);
    svfloat32_t f32 = svcvt_f32_s32_x(svptrue_b32(), r32);
    svfloat32_t f23 = svcvt_f32_s32_x(svptrue_b32(), r23);
    svfloat32_t f33 = svcvt_f32_s32_x(svptrue_b32(), r33);

    // Apply per-column scale.
    svfloat32_t fmul20 = svmul_f32_x(svptrue_b32(), f20, scale0);
    svfloat32_t fmul30 = svmul_f32_x(svptrue_b32(), f30, scale1);
    svfloat32_t fmul21 = svmul_f32_x(svptrue_b32(), f21, scale0);
    svfloat32_t fmul31 = svmul_f32_x(svptrue_b32(), f31, scale1);
    svfloat32_t fmul22 = svmul_f32_x(svptrue_b32(), f22, scale0);
    svfloat32_t fmul32 = svmul_f32_x(svptrue_b32(), f32, scale1);
    svfloat32_t fmul23 = svmul_f32_x(svptrue_b32(), f23, scale0);
    svfloat32_t fmul33 = svmul_f32_x(svptrue_b32(), f33, scale1);

    // frintn (round to nearest int, ties to even), then fcvtzs (fp32->s32).
    svint32_t s20 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul20));
    svint32_t s30 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul30));
    svint32_t s21 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul21));
    svint32_t s31 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul31));
    svint32_t s22 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul22));
    svint32_t s32 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul32));
    svint32_t s23 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul23));
    svint32_t s33 =
        svcvt_s32_x(svptrue_b32(), svrintn_f32_x(svptrue_b32(), fmul33));

    // Add zero point.
    svint32_t zp20 = svadd_s32_x(svptrue_b32(), s20, zp);
    svint32_t zp30 = svadd_s32_x(svptrue_b32(), s30, zp);
    svint32_t zp21 = svadd_s32_x(svptrue_b32(), s21, zp);
    svint32_t zp31 = svadd_s32_x(svptrue_b32(), s31, zp);
    svint32_t zp22 = svadd_s32_x(svptrue_b32(), s22, zp);
    svint32_t zp32 = svadd_s32_x(svptrue_b32(), s32, zp);
    svint32_t zp23 = svadd_s32_x(svptrue_b32(), s23, zp);
    svint32_t zp33 = svadd_s32_x(svptrue_b32(), s33, zp);

    // Clamp.
    svint32_t v20 = svmin_s32_x(svptrue_b32(), zp20, max);
    v20 = svmax_s32_x(svptrue_b32(), v20, min);
    svint32_t v30 = svmin_s32_x(svptrue_b32(), zp30, max);
    v30 = svmax_s32_x(svptrue_b32(), v30, min);
    svint32_t v21 = svmin_s32_x(svptrue_b32(), zp21, max);
    v21 = svmax_s32_x(svptrue_b32(), v21, min);
    svint32_t v31 = svmin_s32_x(svptrue_b32(), zp31, max);
    v31 = svmax_s32_x(svptrue_b32(), v31, min);
    svint32_t v22 = svmin_s32_x(svptrue_b32(), zp22, max);
    v22 = svmax_s32_x(svptrue_b32(), v22, min);
    svint32_t v32 = svmin_s32_x(svptrue_b32(), zp32, max);
    v32 = svmax_s32_x(svptrue_b32(), v32, min);
    svint32_t v23 = svmin_s32_x(svptrue_b32(), zp23, max);
    v23 = svmax_s32_x(svptrue_b32(), v23, min);
    svint32_t v33 = svmin_s32_x(svptrue_b32(), zp33, max);
    v33 = svmax_s32_x(svptrue_b32(), v33, min);

    // Reinterpret s32 -> s16, then uzp1 to interleave low halfwords.
    svint16_t h0 =
        svuzp1_s16(svreinterpret_s16_s32(v20), svreinterpret_s16_s32(v30));
    svint16_t h1 =
        svuzp1_s16(svreinterpret_s16_s32(v21), svreinterpret_s16_s32(v31));
    svint16_t h2 =
        svuzp1_s16(svreinterpret_s16_s32(v22), svreinterpret_s16_s32(v32));
    svint16_t h3 =
        svuzp1_s16(svreinterpret_s16_s32(v23), svreinterpret_s16_s32(v33));

    int8_t *addr2 = tile_base + slice * row_stride + tile2_offset;
    // Truncating stores of low 8 bits.
    svst1b_s16(svptrue_b8(), addr2, h0);
    svst1b_s16(svptrue_b8(), addr2 + row_stride, h1);
    svst1b_s16(svptrue_b8(), addr2 + 2 * row_stride, h2);
    svst1b_s16(svptrue_b8(), addr2 + 3 * row_stride, h3);
  }
}

// Read fp32 data from ZA accumulator to memory.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_readacc_f32(
    float *tile_base, size_t row_stride) __arm_streaming __arm_in("za") {
  uint64_t tile2_offset = svcntw() * row_stride;

  for (uint64_t slice = 0; slice < svcntw(); ++slice) {
    // Read horizontal slices from ZA tiles.
    svfloat32_t r0 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 0, slice);
    svfloat32_t r1 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 1, slice);
    svfloat32_t r2 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 2, slice);
    svfloat32_t r3 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 3, slice);

    // Compute memory addresses.
    uint64_t slice_offset = slice * row_stride;
    float *addr0 = tile_base + slice_offset;
    float *addr1 = addr0 + svcntw();
    float *addr2 = addr0 + tile2_offset;
    float *addr3 = addr2 + svcntw();

    // Store vector slices.
    svst1_f32(svptrue_b32(), addr0, r0);
    svst1_f32(svptrue_b32(), addr1, r1);
    svst1_f32(svptrue_b32(), addr2, r2);
    svst1_f32(svptrue_b32(), addr3, r3);
  }
}

// Read fp32 data(clamped) from ZA accumulator to memory.
RIPPLE_SME_CANDIDATE_INLINE void ripple_fx_t32x32_sme_readacc_w_clamp_f32(
    float *tile_base, size_t row_stride, float clamp_min,
    float clamp_max) __arm_streaming __arm_in("za") {
  svfloat32_t vmin = svdup_f32(clamp_min);
  svfloat32_t vmax = svdup_f32(clamp_max);
  uint64_t tile2_offset = svcntw() * row_stride;

  for (uint64_t slice = 0; slice < svcntw(); ++slice) {
    // Read horizontal slices from ZA tiles.
    svfloat32_t r0 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 0, slice);
    svfloat32_t r1 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 1, slice);
    svfloat32_t r2 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 2, slice);
    svfloat32_t r3 = svread_hor_za32_m(svundef_f32(), svptrue_b32(), 3, slice);

    // Clamp each slice.
    r0 = svmax_f32_z(svptrue_b32(), svmin_f32_z(svptrue_b32(), r0, vmax), vmin);
    r1 = svmax_f32_z(svptrue_b32(), svmin_f32_z(svptrue_b32(), r1, vmax), vmin);
    r2 = svmax_f32_z(svptrue_b32(), svmin_f32_z(svptrue_b32(), r2, vmax), vmin);
    r3 = svmax_f32_z(svptrue_b32(), svmin_f32_z(svptrue_b32(), r3, vmax), vmin);

    // Compute memory addresses.
    uint64_t slice_offset = slice * row_stride;
    float *addr0 = tile_base + slice_offset;
    float *addr1 = addr0 + svcntw();
    float *addr2 = addr0 + tile2_offset;
    float *addr3 = addr2 + svcntw();

    // Store clamped vector slices.
    svst1_f32(svptrue_b32(), addr0, r0);
    svst1_f32(svptrue_b32(), addr1, r1);
    svst1_f32(svptrue_b32(), addr2, r2);
    svst1_f32(svptrue_b32(), addr3, r3);
  }
}

#ifdef __cplusplus
}
#endif

#undef RIPPLE_SME_CANDIDATE_INLINE
#undef RIPPLE_API_FIXED_WLANES
