//===--------- ripple_sme_veclib.h - Ripple SME runtime helpers -----------===//
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

// Internal linkage permits inclusion in multiple C and C++ translation units.
// Do not force inlining across incompatible SME streaming interfaces.
#define RIPPLE_SME_CANDIDATE_INLINE                                            \
  static inline __attribute__((ripple_vector_candidate, unused))

// Our vscale assumption.
#define RIPPLE_API_FIXED_VSCALE_STREAMING 4
// There is no macro for streaming SVE bits, unlike __ARM_FEATURE_SVE_BITS for
// non-streaming.
#define SSVE_BYTES (512 /* bits */ / 8)
#define VEC_ELEMS_I8 (SSVE_BYTES / sizeof(int8_t))
#define VEC_ELEMS_I32 (SSVE_BYTES / sizeof(int32_t))

#ifdef __cplusplus
extern "C" {
#endif

// Verify if Ripple operation fixed vscale is same as runtime vscale.
static inline bool verify_ripple_sme_api_fixed_vscale(void) {
  const uint64_t NeonBytes = 16;
  return svcntsb() == (NeonBytes * RIPPLE_API_FIXED_VSCALE_STREAMING);
}

///////////////  Scalar function declarations  //////////////

/// Zero all SME tiles. Dummy return value.
int32_t zeroAccumulator_i32(ripple_block_t) __arm_streaming __arm_out("za");
/// Zero all SME tiles. Dummy return value.
float zeroAccumulator_f32(ripple_block_t) __arm_streaming __arm_out("za");

/// Add a vector to all rows in the SME tiles.
void addHorizToAccumulator_i32(int32_t Data) __arm_streaming __arm_inout("za");

/// Get horizontal slice (2 vectors) from the SME tile.
/// +-----+-----+
/// | za0 | za1 |
/// +-----+-----+
/// | za2 | za3 |
/// +-----+-----+
/// getAccumHorizSlice(slice=x) returns {za0.s[x], za1.s[x]}
/// or if x >= VEC_ELEMS_I32 {za2.s[x], za3.s[x]}
/// Slice should be <= VEC_ELEMS_I32.
int32_t getAccumHorizSlice_i32(ripple_block_t,
                               uint32_t Slice) __arm_streaming __arm_in("za");

/// Get horizontal slice (2 vectors) from the SME tile.
/// +-----+-----+
/// | za0 | za1 |
/// +-----+-----+
/// | za2 | za3 |
/// +-----+-----+
/// getAccumHorizSlice(slice=x) returns {za0.s[x], za1.s[x]}
/// or if x >= VEC_ELEMS_I32 {za2.s[x], za3.s[x]}
/// Slice should be <= VEC_ELEMS_I32.
float getAccumHorizSlice_f32(ripple_block_t,
                             uint32_t Slice) __arm_streaming __arm_in("za");

/// Outer Product Accumulate. 4-way I8->I32 signed.
void outerProductAccumulate_i8(int8_t LHS,
                               int8_t RHS) __arm_streaming __arm_inout("za");
/// Outer Product Accumulate. F32->F32.
void outerProductAccumulate_f32(float LHS,
                                float RHS) __arm_streaming __arm_inout("za");

///////////////  Types for internal use  //////////////
// Vector types.
typedef int8_t NEON_SSVL_I8_VT __attribute__((__vector_size__(SSVE_BYTES)));
typedef int8_t NEON_SSVL_I8_VT_x2
    __attribute__((__vector_size__(SSVE_BYTES * 2)));
typedef int32_t NEON_SSVL_I32_VT __attribute__((__vector_size__(SSVE_BYTES)));
typedef int32_t NEON_SSVL_I32_VT_x2
    __attribute__((__vector_size__(SSVE_BYTES * 2)));
typedef float NEON_SSVL_F32_VT __attribute__((__vector_size__(SSVE_BYTES)));
typedef float NEON_SSVL_F32_VT_x2
    __attribute__((__vector_size__(SSVE_BYTES * 2)));

// Matrix/2d vector types.
typedef int32_t NEON_SSVL_MT_I32
    __attribute__((__vector_size__(SSVE_BYTES * VEC_ELEMS_I32)));
typedef int32_t NEON_SSVL_MT_I32_x4
    __attribute__((__vector_size__(SSVE_BYTES * 2 * VEC_ELEMS_I32 * 2)));
typedef float NEON_SSVL_MT_F32
    __attribute__((__vector_size__(SSVE_BYTES * VEC_ELEMS_I32)));
typedef float NEON_SSVL_MT_F32_x4
    __attribute__((__vector_size__(SSVE_BYTES * 2 * VEC_ELEMS_I32 * 2)));

union DoubleVector {
  NEON_SSVL_I8_VT_x2 I8Vector;
  NEON_SSVL_I32_VT_x2 I32Vector;
  NEON_SSVL_F32_VT_x2 F32Vector;
  NEON_SSVL_I8_VT I8Vectors[2];
  NEON_SSVL_I32_VT I32Vectors[2];
  NEON_SSVL_F32_VT F32Vectors[2];
};
typedef union DoubleVector DoubleVector;

///////////////  SME library functions. Vector Implementation.  //////////////

/// Zero the tiles. Dummy return value.
RIPPLE_SME_CANDIDATE_INLINE NEON_SSVL_MT_I32_x4
ripple_ret_t32x32i32_zeroAccumulator_i32(void) __arm_streaming __arm_out("za") {
  svzero_za();
  return (NEON_SSVL_MT_I32_x4){0};
}

/// Zero the tiles. Dummy return value.
RIPPLE_SME_CANDIDATE_INLINE NEON_SSVL_MT_F32_x4
ripple_ret_t32x32f32_zeroAccumulator_f32(void) __arm_streaming __arm_out("za") {
  svzero_za();
  return (NEON_SSVL_MT_F32_x4){0.0f};
}

/// Add two I32 vectors to horizontally to all rows in the SME tiles.
RIPPLE_SME_CANDIDATE_INLINE void ripple_arg0_t32i32_addHorizToAccumulator_i32(
    NEON_SSVL_I32_VT_x2 X) __arm_streaming __arm_inout("za") {
  svbool_t PTrue = svptrue_b8();
  NEON_SSVL_I32_VT X_lo = (DoubleVector){X}.I32Vectors[0];
  NEON_SSVL_I32_VT X_hi = (DoubleVector){X}.I32Vectors[1];
  svaddha_za32_s32_m(0, PTrue, PTrue, X_lo);
  svaddha_za32_s32_m(1, PTrue, PTrue, X_hi);
  svaddha_za32_s32_m(2, PTrue, PTrue, X_lo);
  svaddha_za32_s32_m(3, PTrue, PTrue, X_hi);
}

/// Outer Product Accumulate. 4-way I8->I32 signed.
RIPPLE_SME_CANDIDATE_INLINE void
ripple_ew_arg0_t128i8_arg1_t128i8_outerProductAccumulate_i8(
    NEON_SSVL_I8_VT_x2 X,
    NEON_SSVL_I8_VT_x2 Y) __arm_streaming __arm_inout("za") {
  svbool_t PTrue = svptrue_b8();
  NEON_SSVL_I8_VT LHS_lo = (DoubleVector){X}.I8Vectors[0];
  NEON_SSVL_I8_VT LHS_hi = (DoubleVector){X}.I8Vectors[1];

  NEON_SSVL_I8_VT RHS_lo = (DoubleVector){Y}.I8Vectors[0];
  NEON_SSVL_I8_VT RHS_hi = (DoubleVector){Y}.I8Vectors[1];

  svmopa_za32_m(0, PTrue, PTrue, LHS_lo, RHS_lo);
  svmopa_za32_m(1, PTrue, PTrue, LHS_lo, RHS_hi);
  svmopa_za32_m(2, PTrue, PTrue, LHS_hi, RHS_lo);
  svmopa_za32_m(3, PTrue, PTrue, LHS_hi, RHS_hi);
}

/// Outer Product Accumulate. F32.
RIPPLE_SME_CANDIDATE_INLINE void
ripple_ew_arg0_t32f32_arg1_t32f32_outerProductAccumulate_f32(
    NEON_SSVL_F32_VT_x2 X,
    NEON_SSVL_F32_VT_x2 Y) __arm_streaming __arm_inout("za") {
  svbool_t PTrue = svptrue_b32();
  NEON_SSVL_F32_VT LHS_lo = (DoubleVector){X}.F32Vectors[0];
  NEON_SSVL_F32_VT LHS_hi = (DoubleVector){X}.F32Vectors[1];

  NEON_SSVL_F32_VT RHS_lo = (DoubleVector){Y}.F32Vectors[0];
  NEON_SSVL_F32_VT RHS_hi = (DoubleVector){Y}.F32Vectors[1];

  svmopa_za32_m(0, PTrue, PTrue, LHS_lo, RHS_lo);
  svmopa_za32_m(1, PTrue, PTrue, LHS_lo, RHS_hi);
  svmopa_za32_m(2, PTrue, PTrue, LHS_hi, RHS_lo);
  svmopa_za32_m(3, PTrue, PTrue, LHS_hi, RHS_hi);
}

/// Get horizontal slice (2 vectors) from the SME tile.
/// There are 4 tiles for 32-bit data type, and just one for 8bit.
/// Tile has to be accessed using a constant integer name (za0, za1...) and a
/// variable slice number. The tiles are visually abstracted to something like
/// this: For 8bit:
/// +-----+-----+
/// |           |
/// +    za0    +
/// |           |
/// +-----+-----+
/// For 32bit:
/// +-----+-----+
/// | za0 | za1 |
/// +-----+-----+
/// | za2 | za3 |
/// +-----+-----+
/// We want the function(slice=x) to return {za0.s[x], za1.s[x]}
/// or if x >= VEC_ELEMS_I32 {za2.s[x], za3.s[x]}
/// Slice should be <= VEC_ELEMS_I32 or it'll an out of bounds access.
///
/// In reality data is interleaved, and we can just read from za0.b and
/// reinterpret to 32bit, for 32bit data (.s) the layout is:
/// za0.s[0]  ==  za0.b[0]
/// za1.s[0]  ==  za0.b[1]
/// za2.s[0]  ==  za0.b[2]
/// za3.s[0]  ==  za0.b[3]
/// za0.s[1]  ==  za0.b[4]
/// za1.s[1]  ==  za0.b[5]
/// za2.s[1]  ==  za0.b[6]
/// za3.s[1]  ==  za0.b[7]
RIPPLE_SME_CANDIDATE_INLINE NEON_SSVL_I32_VT_x2
ripple_pure_ret_t32i32_getAccumHorizSlice_i32(
    uint32_t Slice) __arm_streaming __arm_in("za") {
  uint32_t ByteTileIdx = (Slice % VEC_ELEMS_I32) * 4;
  // For za2 and za3 slice is >= VEC_ELEMS_I32, so effectively add 2.
  ByteTileIdx += (Slice / VEC_ELEMS_I32) * 2;

  svbool_t PTrue = svptrue_b8();
  DoubleVector DoubleVec;
  DoubleVec.I32Vectors[0] = svreinterpret_s32_s8(
      svread_hor_za8_m(svdup_s8(0), PTrue, 0, ByteTileIdx));
  DoubleVec.I32Vectors[1] = svreinterpret_s32_s8(
      svread_hor_za8_m(svdup_s8(0), PTrue, 0, ByteTileIdx + 1));
  return (NEON_SSVL_I32_VT_x2)DoubleVec.I32Vector;
}

/// Get horizontal slice (2 vectors) from the SME tile.
RIPPLE_SME_CANDIDATE_INLINE NEON_SSVL_F32_VT_x2
ripple_pure_ret_t32f32_getAccumHorizSlice_f32(
    uint32_t Slice) __arm_streaming __arm_in("za") {
  uint32_t ByteTileIdx = (Slice % VEC_ELEMS_I32) * 4;
  // For za2 and za3 slice is >= VEC_ELEMS_I32, so effectively add 2.
  ByteTileIdx += (Slice / VEC_ELEMS_I32) * 2;

  svbool_t PTrue = svptrue_b8();
  DoubleVector DoubleVec;
  DoubleVec.F32Vectors[0] = svreinterpret_f32_s8(
      svread_hor_za8_m(svdup_s8(0), PTrue, 0, ByteTileIdx));
  DoubleVec.F32Vectors[1] = svreinterpret_f32_s8(
      svread_hor_za8_m(svdup_s8(0), PTrue, 0, ByteTileIdx + 1));
  return (NEON_SSVL_F32_VT_x2)DoubleVec.F32Vector;
}

#ifdef __cplusplus
}
#endif

#undef RIPPLE_SME_CANDIDATE_INLINE
#undef UTIL_FUNC_ATTR
#undef RIPPLE_API_FIXED_VSCALE_STREAMING
#undef SSVE_BYTES
#undef VEC_ELEMS_I8
#undef VEC_ELEMS_I32
