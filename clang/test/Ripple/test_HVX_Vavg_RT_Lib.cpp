// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -emit-llvm %s -o - 2>&1 | FileCheck %s

// This test verifies that the Ripple compiler pass correctly replaces calls to
// the polymorphic hvx_vavg / hvx_vavg_rnd / hvx_vnavg entry points (declared
// in HVX_Vavg.h) with the corresponding ripple_ew_pure_hvx_* RT library
// implementations.
//
// Each gen_check_* macro instantiates a function that:
//   1. Sets a block shape to match the HVX vector width (VEC elements).
//   2. Marks the loop as Ripple-parallel so the pass can vectorize it.
//   3. Calls the polymorphic hvx_v* function element-wise; the pass should
//      replace the scalar call with a single HVX vector intrinsic call.
//
// The FileCheck lines verify the expected @ripple_ew_pure_hvx_* symbol appears
// in the emitted LLVM IR.

#include "ripple_test.h"
// HVX_Vavg RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
#include "ripple_hvx_test_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define _decl_hvx_vavg(cOutType, outType, cInType, inType)                     \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vavg_##inType##inType(cInType a, cInType b)

#define _decl_hvx_vavg_rnd(cOutType, outType, cInType, inType)                 \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vavg_##inType##inType##_rnd(cInType a, cInType b)

#define _decl_hvx_vnavg(cOutType, outType, cInType, inType)                    \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vnavg_##inType##inType(cInType a, cInType b)

_decl_hvx_vavg(unsigned char, u8, unsigned char, u8);
_decl_hvx_vavg(signed char, i8, signed char, i8);
_decl_hvx_vavg(unsigned short, u16, unsigned short, u16);
_decl_hvx_vavg(short, i16, short, i16);
_decl_hvx_vavg(unsigned int, u32, unsigned int, u32);
_decl_hvx_vavg(int, i32, int, i32);

_decl_hvx_vavg_rnd(unsigned char, u8, unsigned char, u8);
_decl_hvx_vavg_rnd(signed char, i8, signed char, i8);
_decl_hvx_vavg_rnd(unsigned short, u16, unsigned short, u16);
_decl_hvx_vavg_rnd(short, i16, short, i16);
_decl_hvx_vavg_rnd(unsigned int, u32, unsigned int, u32);
_decl_hvx_vavg_rnd(int, i32, int, i32);

_decl_hvx_vnavg(signed char, i8, signed char, i8);
_decl_hvx_vnavg(signed char, i8, unsigned char, u8);
_decl_hvx_vnavg(short, i16, short, i16);
_decl_hvx_vnavg(int, i32, int, i32);

#undef _decl_hvx_vavg
#undef _decl_hvx_vavg_rnd
#undef _decl_hvx_vnavg

extern RIPPLE_VECTOR_CANDIDATE v128i8 ripple_ew_pure_hvx_i8_vavg_i8i8(v128i8 a,
                                                                      v128i8 b);
extern RIPPLE_VECTOR_CANDIDATE v128i8
ripple_ew_pure_hvx_i8_vavg_i8i8_rnd(v128i8 a, v128i8 b);
extern RIPPLE_VECTOR_CANDIDATE v128i8
ripple_ew_pure_hvx_i8_vnavg_i8i8(v128i8 a, v128i8 b);
extern RIPPLE_VECTOR_CANDIDATE v128u8 ripple_ew_pure_hvx_u8_vavg_u8u8(v128u8 a,
                                                                      v128u8 b);
extern RIPPLE_VECTOR_CANDIDATE v128u8
ripple_ew_pure_hvx_u8_vavg_u8u8_rnd(v128u8 a, v128u8 b);
extern RIPPLE_VECTOR_CANDIDATE v128i8
ripple_ew_pure_hvx_i8_vnavg_u8u8(v128u8 a, v128u8 b);
extern RIPPLE_VECTOR_CANDIDATE v64u16
ripple_ew_pure_hvx_u16_vavg_u16u16(v64u16 a, v64u16 b);
extern RIPPLE_VECTOR_CANDIDATE v64u16
ripple_ew_pure_hvx_u16_vavg_u16u16_rnd(v64u16 a, v64u16 b);
extern RIPPLE_VECTOR_CANDIDATE v64i16
ripple_ew_pure_hvx_i16_vavg_i16i16(v64i16 a, v64i16 b);
extern RIPPLE_VECTOR_CANDIDATE v64i16
ripple_ew_pure_hvx_i16_vavg_i16i16_rnd(v64i16 a, v64i16 b);
extern RIPPLE_VECTOR_CANDIDATE v64i16
ripple_ew_pure_hvx_i16_vnavg_i16i16(v64i16 a, v64i16 b);
extern RIPPLE_VECTOR_CANDIDATE v32u32
ripple_ew_pure_hvx_u32_vavg_u32u32(v32u32 a, v32u32 b);
extern RIPPLE_VECTOR_CANDIDATE v32u32
ripple_ew_pure_hvx_u32_vavg_u32u32_rnd(v32u32 a, v32u32 b);
extern RIPPLE_VECTOR_CANDIDATE v32i32
ripple_ew_pure_hvx_i32_vavg_i32i32(v32i32 a, v32i32 b);
extern RIPPLE_VECTOR_CANDIDATE v32i32
ripple_ew_pure_hvx_i32_vavg_i32i32_rnd(v32i32 a, v32i32 b);
extern RIPPLE_VECTOR_CANDIDATE v32i32
ripple_ew_pure_hvx_i32_vnavg_i32i32(v32i32 a, v32i32 b);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

#define hvx_vavg(a, b)                                                         \
  _Generic((a),                                                                \
      char: hvx_u8_vavg_u8u8((unsigned char)(a), (unsigned char)(b)),          \
      signed char: hvx_i8_vavg_i8i8((a), (b)),                                 \
      unsigned char: hvx_u8_vavg_u8u8((a), (b)),                               \
      signed short: hvx_i16_vavg_i16i16((a), (b)),                             \
      unsigned short: hvx_u16_vavg_u16u16((a), (b)),                           \
      signed int: hvx_i32_vavg_i32i32((a), (b)),                               \
      unsigned int: hvx_u32_vavg_u32u32((a), (b)),                             \
      signed long: hvx_i32_vavg_i32i32((int)(a), (int)(b)),                    \
      unsigned long: hvx_u32_vavg_u32u32((unsigned int)(a),                    \
                                         (unsigned int)(b)))

#define hvx_vavg_rnd(a, b)                                                     \
  _Generic((a),                                                                \
      char: hvx_u8_vavg_u8u8_rnd((unsigned char)(a), (unsigned char)(b)),      \
      signed char: hvx_i8_vavg_i8i8_rnd((a), (b)),                             \
      unsigned char: hvx_u8_vavg_u8u8_rnd((a), (b)),                           \
      signed short: hvx_i16_vavg_i16i16_rnd((a), (b)),                         \
      unsigned short: hvx_u16_vavg_u16u16_rnd((a), (b)),                       \
      signed int: hvx_i32_vavg_i32i32_rnd((a), (b)),                           \
      unsigned int: hvx_u32_vavg_u32u32_rnd((a), (b)),                         \
      signed long: hvx_i32_vavg_i32i32_rnd((int)(a), (int)(b)),                \
      unsigned long: hvx_u32_vavg_u32u32_rnd((unsigned int)(a),                \
                                             (unsigned int)(b)))

#define hvx_vnavg(a, b)                                                        \
  _Generic((a),                                                                \
      char: hvx_i8_vnavg_u8u8((unsigned char)(a), (unsigned char)(b)),         \
      signed char: hvx_i8_vnavg_i8i8((a), (b)),                                \
      unsigned char: hvx_i8_vnavg_u8u8((a), (b)),                              \
      signed short: hvx_i16_vnavg_i16i16((a), (b)),                            \
      signed int: hvx_i32_vnavg_i32i32((a), (b)),                              \
      signed long: hvx_i32_vnavg_i32i32((int)(a), (int)(b)))

#else

#define _decl_hvx_vavg_spec(CT, UI, SZ)                                        \
  [[gnu::always_inline, gnu::unused]] static CT hvx_vavg(CT a, CT b) {         \
    return hvx_##UI##SZ##_vavg_##UI##SZ##UI##SZ(a, b);                         \
  }

#define _decl_hvx_vavg_rnd_spec(CT, UI, SZ)                                    \
  [[gnu::always_inline, gnu::unused]] static CT hvx_vavg_rnd(CT a, CT b) {     \
    return hvx_##UI##SZ##_vavg_##UI##SZ##UI##SZ##_rnd(a, b);                   \
  }

[[gnu::always_inline, gnu::unused]] static char hvx_vavg(char a, char b) {
  return hvx_u8_vavg_u8u8(a, b);
}
_decl_hvx_vavg_spec(signed char, i, 8);
_decl_hvx_vavg_spec(unsigned char, u, 8);
_decl_hvx_vavg_spec(signed short, i, 16);
_decl_hvx_vavg_spec(unsigned short, u, 16);
_decl_hvx_vavg_spec(signed int, i, 32);
_decl_hvx_vavg_spec(unsigned int, u, 32);
[[gnu::always_inline, gnu::unused]] static signed long hvx_vavg(signed long a,
                                                                signed long b) {
  return hvx_i32_vavg_i32i32((int)a, (int)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_vavg(unsigned long a, unsigned long b) {
  return hvx_u32_vavg_u32u32((unsigned int)a, (unsigned int)b);
}

[[gnu::always_inline, gnu::unused]] static char hvx_vavg_rnd(char a, char b) {
  return hvx_u8_vavg_u8u8_rnd(a, b);
}
_decl_hvx_vavg_rnd_spec(signed char, i, 8);
_decl_hvx_vavg_rnd_spec(unsigned char, u, 8);
_decl_hvx_vavg_rnd_spec(signed short, i, 16);
_decl_hvx_vavg_rnd_spec(unsigned short, u, 16);
_decl_hvx_vavg_rnd_spec(signed int, i, 32);
_decl_hvx_vavg_rnd_spec(unsigned int, u, 32);
[[gnu::always_inline, gnu::unused]] static signed long
hvx_vavg_rnd(signed long a, signed long b) {
  return hvx_i32_vavg_i32i32_rnd((int)a, (int)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned long
hvx_vavg_rnd(unsigned long a, unsigned long b) {
  return hvx_u32_vavg_u32u32_rnd((unsigned int)a, (unsigned int)b);
}

#undef _decl_hvx_vavg_spec
#undef _decl_hvx_vavg_rnd_spec

[[gnu::always_inline, gnu::unused]] static signed char hvx_vnavg(char a,
                                                                 char b) {
  return hvx_i8_vnavg_u8u8(a, b);
}
[[gnu::always_inline, gnu::unused]] static signed char
hvx_vnavg(signed char a, signed char b) {
  return hvx_i8_vnavg_i8i8(a, b);
}
[[gnu::always_inline, gnu::unused]] static signed char
hvx_vnavg(unsigned char a, unsigned char b) {
  return hvx_i8_vnavg_u8u8(a, b);
}
[[gnu::always_inline, gnu::unused]] static short hvx_vnavg(short a, short b) {
  return hvx_i16_vnavg_i16i16(a, b);
}
[[gnu::always_inline, gnu::unused]] static int hvx_vnavg(int a, int b) {
  return hvx_i32_vnavg_i32i32(a, b);
}
[[gnu::always_inline, gnu::unused]] static signed long
hvx_vnavg(signed long a, signed long b) {
  return hvx_i32_vnavg_i32i32((int)a, (int)b);
}

#endif

// N   - total number of elements in the input/output arrays
// VEC - HVX vector width in elements (must equal N for a single vector op)
#define gen_check_vavg(N, VEC, cType, tag)                                     \
  void check_##tag##_vavg(const cType a[N], const cType b[N], cType out[N]) { \
    ripple_block_t BS = ripple_set_block_shape(0, VEC);                        \
    ripple_parallel(BS, 0);                                                    \
    for (int i = 0; i < N; i++)                                                \
      out[i] = hvx_vavg(a[i], b[i]);                                          \
  }

#define gen_check_vavg_rnd(N, VEC, cType, tag)                                 \
  void check_##tag##_vavg_rnd(const cType a[N], const cType b[N],             \
                               cType out[N]) {                                 \
    ripple_block_t BS = ripple_set_block_shape(0, VEC);                        \
    ripple_parallel(BS, 0);                                                    \
    for (int i = 0; i < N; i++)                                                \
      out[i] = hvx_vavg_rnd(a[i], b[i]);                                      \
  }

// vnavg output is always signed regardless of input signedness.
#define gen_check_vnavg(N, VEC, cInType, cOutType, tag)                        \
  void check_##tag##_vnavg(const cInType a[N], const cInType b[N],            \
                            cOutType out[N]) {                                 \
    ripple_block_t BS = ripple_set_block_shape(0, VEC);                        \
    ripple_parallel(BS, 0);                                                    \
    for (int i = 0; i < N; i++)                                                \
      out[i] = hvx_vnavg(a[i], b[i]);                                         \
  }

// =============================================================================
// hvx_vavg — truncating average
// =============================================================================

// --- unsigned ---
gen_check_vavg(256, 128, unsigned char, u8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_u8_vavg_u8u8

gen_check_vavg(128, 64, unsigned short, u16);
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_u16_vavg_u16u16

gen_check_vavg(64, 32, unsigned int, u32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_u32_vavg_u32u32

// --- signed ---
gen_check_vavg(256, 128, signed char, i8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_i8_vavg_i8i8

gen_check_vavg(128, 64, short, i16);
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vavg_i16i16

gen_check_vavg(64, 32, int, i32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vavg_i32i32

// --- long (32-bit on Hexagon, reuses i32/u32 implementations) ---
gen_check_vavg(64, 32, unsigned long, ul32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_u32_vavg_u32u32

gen_check_vavg(64, 32, signed long, sl32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vavg_i32i32

// --- char (always unsigned on Hexagon, dispatches to u8) ---
gen_check_vavg(256, 128, char, char);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_u8_vavg_u8u8

// =============================================================================
// hvx_vavg_rnd — rounding average
// =============================================================================

// --- unsigned ---
gen_check_vavg_rnd(256, 128, unsigned char, u8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_u8_vavg_u8u8_rnd

gen_check_vavg_rnd(128, 64, unsigned short, u16);
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_u16_vavg_u16u16_rnd

gen_check_vavg_rnd(64, 32, unsigned int, u32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_u32_vavg_u32u32_rnd

// --- signed ---
gen_check_vavg_rnd(256, 128, signed char, i8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_i8_vavg_i8i8_rnd

gen_check_vavg_rnd(128, 64, short, i16);
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vavg_i16i16_rnd

gen_check_vavg_rnd(64, 32, int, i32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vavg_i32i32_rnd

// --- char (always unsigned on Hexagon, dispatches to u8) ---
gen_check_vavg_rnd(256, 128, char, char);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_u8_vavg_u8u8_rnd

// --- long (32-bit on Hexagon, reuses i32/u32 implementations) ---
gen_check_vavg_rnd(64, 32, unsigned long, ul32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_u32_vavg_u32u32_rnd

gen_check_vavg_rnd(64, 32, signed long, sl32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vavg_i32i32_rnd

// =============================================================================
// hvx_vnavg — negated average, output is always signed
// =============================================================================

// vnavg computes (a - b) >> 1.
// The u8 variant takes two unsigned 8-bit inputs and produces a signed i8
// result; this is a distinct HVX intrinsic (Q6_Vb_vnavg_VubVub) from the
// signed i8/i8 variant (Q6_Vb_vnavg_VbVb), not a synonym for it.

gen_check_vnavg(256, 128, signed char, signed char, i8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_i8_vnavg_i8i8

gen_check_vnavg(256, 128, unsigned char, signed char, u8);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_i8_vnavg_u8u8

gen_check_vnavg(128, 64, short, short, i16);
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vnavg_i16i16

gen_check_vnavg(64, 32, int, int, i32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vnavg_i32i32

// --- long (32-bit on Hexagon, reuses i32 implementation) ---
gen_check_vnavg(64, 32, signed long, signed long, sl32);
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vnavg_i32i32

// --- char (always unsigned on Hexagon, dispatches to u8 vnavg variant) ---
gen_check_vnavg(256, 128, char, signed char, char);
// CHECK: call <128 x i8> @ripple_ew_pure_hvx_i8_vnavg_u8u8
