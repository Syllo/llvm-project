// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -emit-llvm %s -o - 2>&1 | FileCheck %s

// This test verifies that the Ripple compiler pass correctly replaces calls to
// the hvx_*_vmpy_* / hvx_*_vmpyacc_* API functions (declared in
// HVX_Vmpy_Scalar.h) with the corresponding ripple_ew_pure_hvx_*
// implementations.
//
// Section 1: typed names (hvx_i16_vmpy_u8i8, etc.) — one function per intrinsic.
// Section 2: polymorphic names (hvx_vmpy, hvx_vmpyacc, etc.) — one function
//   per supported type, dispatching through the _Generic / overload wrappers.
//
// Each test function uses ripple_id to index directly into the arrays so the
// Ripple pass can recognize and vectorize the call.  The block shape is set to
// the output vector width (VEC elements of cOutType).
//
// The FileCheck lines verify the expected @ripple_ew_pure_hvx_* symbol appears
// in the emitted LLVM IR.

#include "ripple_test.h"
// HVX_Vmpy_Scalar RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
#include "ripple_hvx_test_types.h"

#ifdef __cplusplus
extern "C" {
#endif

static_assert(sizeof(signed char) == 1 && sizeof(short) == 2,
              "HVX_Vmpy_Scalar: unexpected char/short size");
static_assert(sizeof(int) == 4, "HVX_Vmpy_Scalar: unexpected int size");
static_assert(sizeof(unsigned int) == 4,
              "HVX_Vmpy_Scalar: unexpected unsigned int size");

#define _decl_hvx_vmpy(cOutType, outType, cVecType, vecType, cScType, scType)  \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vmpy_##vecType##scType(cVecType a, cScType b)

_decl_hvx_vmpy(short, i16, unsigned char, u8, signed char,
               i8);
_decl_hvx_vmpy(unsigned short, u16, unsigned char, u8, unsigned char,
               u8);
_decl_hvx_vmpy(int, i32, short, i16, short, i16);

#define _decl_hvx_vmpy_s1_sat(cType, type)                                     \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpy_##type##type##_s1_sat(cType v, cType r)

#define _decl_hvx_vmpy_s1_rnd_sat(cType, type)                                 \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpy_##type##type##_s1_rnd_sat(cType v, cType r)

_decl_hvx_vmpy_s1_sat(short, i16);
_decl_hvx_vmpy_s1_rnd_sat(short, i16);

#define _decl_hvx_vmpy_noshuff(cOutType, outType, cVecType, vecType, cScType,  \
                               scType)                                         \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vmpy_##vecType##scType##_noshuff(cVecType a, cScType b)

_decl_hvx_vmpy_noshuff(short, i16, unsigned char, u8, signed char,
                       i8);
_decl_hvx_vmpy_noshuff(unsigned short, u16, unsigned char, u8, unsigned char,
                       u8);
_decl_hvx_vmpy_noshuff(int, i32, short, i16, short,
                       i16);

#define _decl_hvx_vmpyacc(cOutType, outType, cAccType, accType, cVecType,      \
                          vecType, cScType, scType)                            \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vmpyacc_##accType##vecType##scType(                      \
          cAccType acc, cVecType a, cScType b)

_decl_hvx_vmpyacc(short, i16, short, i16, unsigned char, u8, signed char,
                  i8);
_decl_hvx_vmpyacc(unsigned short, u16, unsigned short, u16, unsigned char, u8,
                  unsigned char, u8);
_decl_hvx_vmpyacc(unsigned int, u32, unsigned int, u32, unsigned short, u16,
                  unsigned short, u16);
_decl_hvx_vmpyacc(int, i32, int, i32, short, i16, short,
                  i16);

#define _decl_hvx_vmpyacc_sat(cType, type, cVecType, vecType, cScType, scType) \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpyacc_##type##vecType##scType##_sat(                      \
          cType acc, cVecType a, cScType b)

_decl_hvx_vmpyacc_sat(int, i32, short, i16, short,
                      i16);

#define _decl_hvx_vmpyacc_noshuff(cOutType, outType, cAccType, accType,        \
                                  cVecType, vecType, cScType, scType)          \
  __attribute__((always_inline)) extern cOutType                               \
      hvx_##outType##_vmpyacc_##accType##vecType##scType##_noshuff(            \
          cAccType acc, cVecType a, cScType b)

_decl_hvx_vmpyacc_noshuff(short, i16, short, i16, unsigned char, u8,
                          signed char, i8);
_decl_hvx_vmpyacc_noshuff(unsigned short, u16, unsigned short, u16,
                          unsigned char, u8, unsigned char,
                          u8);
_decl_hvx_vmpyacc_noshuff(unsigned int, u32, unsigned int, u32, unsigned short,
                          u16, unsigned short,
                          u16);
_decl_hvx_vmpyacc_noshuff(int, i32, int, i32, short, i16, short,
                          i16);

#define _decl_hvx_vmpyacc_sat_noshuff(cType, type, cVecType, vecType, cScType, \
                                      scType)                                  \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpyacc_##type##vecType##scType##_sat_noshuff(              \
          cType acc, cVecType a, cScType b)

_decl_hvx_vmpyacc_sat_noshuff(int, i32, short, i16, short,
                              i16);

__attribute__((always_inline)) extern unsigned int
hvx_u32_vmpy_u16_2u16(unsigned short in, unsigned int r);
__attribute__((always_inline)) extern unsigned int
hvx_u32_vmpy_u16_2u16_noshuff(unsigned short in, unsigned int r);

#undef _decl_hvx_vmpy
#undef _decl_hvx_vmpy_s1_sat
#undef _decl_hvx_vmpy_s1_rnd_sat
#undef _decl_hvx_vmpy_noshuff
#undef _decl_hvx_vmpyacc
#undef _decl_hvx_vmpyacc_sat
#undef _decl_hvx_vmpyacc_noshuff
#undef _decl_hvx_vmpyacc_sat_noshuff

extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_ew_pure_hvx_i16_vmpy_u8i8(v128u8 v, signed char r);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_ew_pure_hvx_u16_vmpy_u8u8(v128u8 v, unsigned char r);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_ew_pure_hvx_u32_vmpy_u16_2u16(v64u16 v, unsigned int r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpy_i16i16(v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v64i16
ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_sat(v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v64i16
ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_rnd_sat(v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_ew_pure_hvx_i16_vmpy_u8i8_noshuff(v128u8 v, signed char r);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_ew_pure_hvx_u16_vmpy_u8u8_noshuff(v128u8 v, unsigned char r);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_ew_pure_hvx_u32_vmpy_u16_2u16_noshuff(v64u16 v, unsigned int r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpy_i16i16_noshuff(v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8(v128i16 x, v128u8 v, signed char r);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8(v128u16 x, v128u8 v, unsigned char r);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16(v64u32 x, v64u16 v, unsigned short r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16(v64i32 x, v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat(v64i32 x, v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v128i16
ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8_noshuff(v128i16 x, v128u8 v,
                                               signed char r);
extern RIPPLE_VECTOR_CANDIDATE v128u16
ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8_noshuff(v128u16 x, v128u8 v,
                                               unsigned char r);
extern RIPPLE_VECTOR_CANDIDATE v64u32
ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16_noshuff(v64u32 x, v64u16 v,
                                                 unsigned short r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_noshuff(v64i32 x, v64i16 v, short r);
extern RIPPLE_VECTOR_CANDIDATE v64i32
ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat_noshuff(v64i32 x, v64i16 v,
                                                     short r);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

#define hvx_vmpy(a, b)                                                         \
  _Generic((b),                                                                \
      char: hvx_u16_vmpy_u8u8((unsigned char)(a), (unsigned char)(b)),         \
      signed char: hvx_i16_vmpy_u8i8((unsigned char)(a), (b)),                 \
      unsigned char: hvx_u16_vmpy_u8u8((unsigned char)(a), (b)),               \
      short: hvx_i32_vmpy_i16i16((short)(a), (b)))

#define hvx_vmpy_noshuff(a, b)                                                 \
  _Generic((b),                                                                \
      char: hvx_u16_vmpy_u8u8_noshuff((unsigned char)(a), (unsigned char)(b)), \
      signed char: hvx_i16_vmpy_u8i8_noshuff((unsigned char)(a), (b)),         \
      unsigned char: hvx_u16_vmpy_u8u8_noshuff((unsigned char)(a), (b)),       \
      short: hvx_i32_vmpy_i16i16_noshuff((short)(a), (b)))

#define hvx_vmpy_s1_sat(v, r)                                                  \
  _Generic((v), short: hvx_i16_vmpy_i16i16_s1_sat((v), (r)))

#define hvx_vmpy_s1_rnd_sat(v, r)                                              \
  _Generic((v), short: hvx_i16_vmpy_i16i16_s1_rnd_sat((v), (r)))

#define hvx_vmpyacc(acc, a, b)                                                 \
  _Generic((b),                                                                \
      char: hvx_u16_vmpyacc_u16u8u8((unsigned short)(acc), (unsigned char)(a), \
                                    (unsigned char)(b)),                       \
      signed char: hvx_i16_vmpyacc_i16u8i8((short)(acc), (unsigned char)(a),   \
                                           (b)),                               \
      unsigned char: hvx_u16_vmpyacc_u16u8u8((unsigned short)(acc),            \
                                             (unsigned char)(a), (b)),         \
      unsigned short: hvx_u32_vmpyacc_u32u16u16((unsigned int)(acc),           \
                                                (unsigned short)(a), (b)),     \
      short: hvx_i32_vmpyacc_i32i16i16((int)(acc), (short)(a), (b)))

#define hvx_vmpyacc_noshuff(acc, a, b)                                         \
  _Generic((b),                                                                \
      char: hvx_u16_vmpyacc_u16u8u8_noshuff(                                   \
               (unsigned short)(acc), (unsigned char)(a), (unsigned char)(b)), \
      signed char: hvx_i16_vmpyacc_i16u8i8_noshuff((short)(acc),               \
                                                   (unsigned char)(a), (b)),   \
      unsigned char: hvx_u16_vmpyacc_u16u8u8_noshuff((unsigned short)(acc),    \
                                                     (unsigned char)(a), (b)), \
      unsigned short: hvx_u32_vmpyacc_u32u16u16_noshuff(                       \
               (unsigned int)(acc), (unsigned short)(a), (b)),                 \
      short: hvx_i32_vmpyacc_i32i16i16_noshuff((int)(acc), (short)(a), (b)))

#define hvx_vmpyacc_sat(acc, a, b)                                             \
  _Generic((b),                                                                \
      short: hvx_i32_vmpyacc_i32i16i16_sat((int)(acc), (short)(a), (b)))

#define hvx_vmpyacc_sat_noshuff(acc, a, b)                                     \
  _Generic((b),                                                                \
      short: hvx_i32_vmpyacc_i32i16i16_sat_noshuff((int)(acc), (short)(a),     \
                                                   (b)))

#else

[[gnu::always_inline, gnu::unused]] static unsigned short hvx_vmpy(char a,
                                                                   char b) {
  return hvx_u16_vmpy_u8u8((unsigned char)a, (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpy(unsigned char a, char b) {
  return hvx_u16_vmpy_u8u8(a, (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static short hvx_vmpy(unsigned char a,
                                                          signed char b) {
  return hvx_i16_vmpy_u8i8(a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpy(unsigned char a, unsigned char b) {
  return hvx_u16_vmpy_u8u8(a, b);
}
[[gnu::always_inline, gnu::unused]] static int hvx_vmpy(short a, short b) {
  return hvx_i32_vmpy_i16i16(a, b);
}

[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpy_noshuff(char a, char b) {
  return hvx_u16_vmpy_u8u8_noshuff((unsigned char)a, (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpy_noshuff(unsigned char a, char b) {
  return hvx_u16_vmpy_u8u8_noshuff(a, (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static short
hvx_vmpy_noshuff(unsigned char a, signed char b) {
  return hvx_i16_vmpy_u8i8_noshuff(a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpy_noshuff(unsigned char a, unsigned char b) {
  return hvx_u16_vmpy_u8u8_noshuff(a, b);
}
[[gnu::always_inline, gnu::unused]] static int hvx_vmpy_noshuff(short a,
                                                                short b) {
  return hvx_i32_vmpy_i16i16_noshuff(a, b);
}

[[gnu::always_inline, gnu::unused]] static short hvx_vmpy_s1_sat(short v,
                                                                 short r) {
  return hvx_i16_vmpy_i16i16_s1_sat(v, r);
}

[[gnu::always_inline, gnu::unused]] static short hvx_vmpy_s1_rnd_sat(short v,
                                                                     short r) {
  return hvx_i16_vmpy_i16i16_s1_rnd_sat(v, r);
}

[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc(char acc, char a, char b) {
  return hvx_u16_vmpyacc_u16u8u8((unsigned short)acc, (unsigned char)a,
                                 (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static short
hvx_vmpyacc(short acc, unsigned char a, signed char b) {
  return hvx_i16_vmpyacc_i16u8i8(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc(short acc, unsigned char a, char b) {
  return hvx_u16_vmpyacc_u16u8u8((unsigned short)acc, a, (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc(unsigned short acc, unsigned char a, unsigned char b) {
  return hvx_u16_vmpyacc_u16u8u8(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_vmpyacc(unsigned int acc, unsigned short a, unsigned short b) {
  return hvx_u32_vmpyacc_u32u16u16(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static int hvx_vmpyacc(int acc, short a,
                                                           short b) {
  return hvx_i32_vmpyacc_i32i16i16(acc, a, b);
}

[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc_noshuff(char acc, char a, char b) {
  return hvx_u16_vmpyacc_u16u8u8_noshuff((unsigned short)acc, (unsigned char)a,
                                         (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static short
hvx_vmpyacc_noshuff(short acc, unsigned char a, signed char b) {
  return hvx_i16_vmpyacc_i16u8i8_noshuff(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc_noshuff(short acc, unsigned char a, char b) {
  return hvx_u16_vmpyacc_u16u8u8_noshuff((unsigned short)acc, a,
                                         (unsigned char)b);
}
[[gnu::always_inline, gnu::unused]] static unsigned short
hvx_vmpyacc_noshuff(unsigned short acc, unsigned char a, unsigned char b) {
  return hvx_u16_vmpyacc_u16u8u8_noshuff(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static unsigned int
hvx_vmpyacc_noshuff(unsigned int acc, unsigned short a, unsigned short b) {
  return hvx_u32_vmpyacc_u32u16u16_noshuff(acc, a, b);
}
[[gnu::always_inline, gnu::unused]] static int
hvx_vmpyacc_noshuff(int acc, short a, short b) {
  return hvx_i32_vmpyacc_i32i16i16_noshuff(acc, a, b);
}

[[gnu::always_inline, gnu::unused]] static int hvx_vmpyacc_sat(int acc, short a,
                                                               short b) {
  return hvx_i32_vmpyacc_i32i16i16_sat(acc, a, b);
}

[[gnu::always_inline, gnu::unused]] static int
hvx_vmpyacc_sat_noshuff(int acc, short a, short b) {
  return hvx_i32_vmpyacc_i32i16i16_sat_noshuff(acc, a, b);
}

#endif

extern "C" {

// =============================================================================
// Section 1: typed API
// =============================================================================

// --- vmpy: out[v] = a[v] * b (widening, odd/even interleaved output) ---

void check_vmpy_i16_u8i8(size_t len, const unsigned char *a, signed char b,
                          short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpy_u8i8(a[v], b);
}
// CHECK: @check_vmpy_i16_u8i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpy_u8i8

void check_vmpy_u16_u8u8(size_t len, const unsigned char *a, unsigned char b,
                          unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u16_vmpy_u8u8(a[v], b);
}
// CHECK: @check_vmpy_u16_u8u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8

void check_vmpy_i32_i16i16(size_t len, const short *a, short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpy_i16i16(a[v], b);
}
// CHECK: @check_vmpy_i32_i16i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpy_i16i16

// --- vmpy_s1_sat / vmpy_s1_rnd_sat: non-widening, <<1 + sat ---

void check_vmpy_i16_i16i16_s1_sat(size_t len, const short *a, short b,
                                   short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpy_i16i16_s1_sat(a[v], b);
}
// CHECK: @check_vmpy_i16_i16i16_s1_sat
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_sat

void check_vmpy_i16_i16i16_s1_rnd_sat(size_t len, const short *a, short b,
                                       short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpy_i16i16_s1_rnd_sat(a[v], b);
}
// CHECK: @check_vmpy_i16_i16i16_s1_rnd_sat
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_rnd_sat

// --- vmpy_noshuff: widening, sequential output ---

void check_vmpy_i16_u8i8_noshuff(size_t len, const unsigned char *a,
                                  signed char b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpy_u8i8_noshuff(a[v], b);
}
// CHECK: @check_vmpy_i16_u8i8_noshuff
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpy_u8i8_noshuff

void check_vmpy_u16_u8u8_noshuff(size_t len, const unsigned char *a,
                                  unsigned char b, unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u16_vmpy_u8u8_noshuff(a[v], b);
}
// CHECK: @check_vmpy_u16_u8u8_noshuff
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8_noshuff

void check_vmpy_i32_i16i16_noshuff(size_t len, const short *a, short b,
                                    int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpy_i16i16_noshuff(a[v], b);
}
// CHECK: @check_vmpy_i32_i16i16_noshuff
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpy_i16i16_noshuff

// --- vmpyacc: out[v] = acc[v] + a[v] * b (widening, interleaved output) ---

void check_vmpyacc_i16_i16u8i8(size_t len, short *acc, const unsigned char *a,
                                signed char b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpyacc_i16u8i8(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i16_i16u8i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8

void check_vmpyacc_u16_u16u8u8(size_t len, unsigned short *acc,
                                const unsigned char *a, unsigned char b,
                                unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u16_vmpyacc_u16u8u8(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_u16_u16u8u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8

void check_vmpyacc_u32_u32u16u16(size_t len, unsigned int *acc,
                                  const unsigned short *a, unsigned short b,
                                  unsigned int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u32_vmpyacc_u32u16u16(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_u32_u32u16u16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16

void check_vmpyacc_i32_i32i16i16(size_t len, int *acc, const short *a,
                                  short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyacc_i32i16i16(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i32_i32i16i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16

// --- vmpyacc_sat: saturating, interleaved output ---

void check_vmpyacc_i32_i32i16i16_sat(size_t len, int *acc, const short *a,
                                      short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyacc_i32i16i16_sat(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i32_i32i16i16_sat
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat

// --- vmpyacc_noshuff: widening, sequential output ---

void check_vmpyacc_i16u8i8_noshuff(size_t len, short *acc,
                                    const unsigned char *a, signed char b,
                                    short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i16_vmpyacc_i16u8i8_noshuff(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i16u8i8_noshuff
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8_noshuff

void check_vmpyacc_u16u8u8_noshuff(size_t len, unsigned short *acc,
                                    const unsigned char *a, unsigned char b,
                                    unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u16_vmpyacc_u16u8u8_noshuff(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_u16u8u8_noshuff
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8_noshuff

void check_vmpyacc_u32u16u16_noshuff(size_t len, unsigned int *acc,
                                      const unsigned short *a, unsigned short b,
                                      unsigned int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_u32_vmpyacc_u32u16u16_noshuff(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_u32u16u16_noshuff
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16_noshuff

void check_vmpyacc_i32i16i16_noshuff(size_t len, int *acc, const short *a,
                                      short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyacc_i32i16i16_noshuff(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i32i16i16_noshuff
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_noshuff

// --- vmpyacc_sat_noshuff: saturating, sequential output ---

void check_vmpyacc_i32i16i16_sat_noshuff(size_t len, int *acc, const short *a,
                                          short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyacc_i32i16i16_sat_noshuff(acc[v], a[v], b);
}
// CHECK: @check_vmpyacc_i32i16i16_sat_noshuff
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat_noshuff

// =============================================================================
// Section 2: polymorphic API (hvx_vmpy / hvx_vmpyacc / etc.)
// =============================================================================
// Dispatch is on the scalar (last) argument type.
// char is always unsigned on Hexagon, so 'char' dispatches to the u8u8 variant.

// --- hvx_vmpy ---

// signed char scalar -> i16 output (u8i8 variant)
void check_poly_vmpy_i8(const unsigned char *a, signed char b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy(a[v], b);
}
// CHECK: @check_poly_vmpy_i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpy_u8i8

// unsigned char scalar -> u16 output (u8u8 variant)
void check_poly_vmpy_u8(const unsigned char *a, unsigned char b,
                        unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy(a[v], b);
}
// CHECK: @check_poly_vmpy_u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8

// short scalar -> i32 output (i16i16 variant)
void check_poly_vmpy_i16(const short *a, short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy(a[v], b);
}
// CHECK: @check_poly_vmpy_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpy_i16i16

// char scalar, unsigned char vector -> u16 output (u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpy_u8_char(const unsigned char *a, char b,
                              unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy(a[v], b);
}
// CHECK: @check_poly_vmpy_u8_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8

// char scalar, char vector -> u16 output (u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpy_char(const char *a, char b, unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy(a[v], b);
}
// CHECK: @check_poly_vmpy_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8

// --- hvx_vmpy_noshuff ---

void check_poly_vmpy_noshuff_i8(const unsigned char *a, signed char b,
                                 short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_noshuff(a[v], b);
}
// CHECK: @check_poly_vmpy_noshuff_i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpy_u8i8_noshuff

void check_poly_vmpy_noshuff_u8(const unsigned char *a, unsigned char b,
                                 unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_noshuff(a[v], b);
}
// CHECK: @check_poly_vmpy_noshuff_u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8_noshuff

void check_poly_vmpy_noshuff_i16(const short *a, short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_noshuff(a[v], b);
}
// CHECK: @check_poly_vmpy_noshuff_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpy_i16i16_noshuff

void check_poly_vmpy_noshuff_char(const unsigned char *a, char b,
                                  unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_noshuff(a[v], b);
}
// CHECK: @check_poly_vmpy_noshuff_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8_noshuff

// char scalar, char vector -> u16 output (u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpy_noshuff_char_char(const char *a, char b,
                                       unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_noshuff(a[v], b);
}
// CHECK: @check_poly_vmpy_noshuff_char_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpy_u8u8_noshuff

// --- hvx_vmpy_s1_sat ---

void check_poly_vmpy_s1_sat(const short *a, short b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_s1_sat(a[v], b);
}
// CHECK: @check_poly_vmpy_s1_sat
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_sat

// --- hvx_vmpy_s1_rnd_sat ---

void check_poly_vmpy_s1_rnd_sat(const short *a, short b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpy_s1_rnd_sat(a[v], b);
}
// CHECK: @check_poly_vmpy_s1_rnd_sat
// CHECK: call <64 x i16> @ripple_ew_pure_hvx_i16_vmpy_i16i16_s1_rnd_sat

// --- hvx_vmpyacc ---

// signed char scalar -> i16 acc (i16u8i8 variant)
void check_poly_vmpyacc_i8(short *acc, const unsigned char *a, signed char b,
                            short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8

// unsigned char scalar -> u16 acc (u16u8u8 variant)
void check_poly_vmpyacc_u8(unsigned short *acc, const unsigned char *a,
                            unsigned char b, unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8

// unsigned short scalar -> u32 acc (u32u16u16 variant)
void check_poly_vmpyacc_u16(unsigned int *acc, const unsigned short *a,
                             unsigned short b, unsigned int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_u16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16

// short scalar -> i32 acc (i32i16i16 variant)
void check_poly_vmpyacc_i16(int *acc, const short *a, short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16

// char scalar -> u16 acc (u16u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpyacc_char(char *acc, const char *a, char b,
                              unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8

// char scalar, short acc, unsigned char vector -> u16 output (u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpyacc_i16acc_u8_char(short *acc, const unsigned char *a,
                                        char b, unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_i16acc_u8_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8

// --- hvx_vmpyacc_noshuff ---

void check_poly_vmpyacc_noshuff_i8(short *acc, const unsigned char *a,
                                    signed char b, short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_i8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_i16_vmpyacc_i16u8i8_noshuff

void check_poly_vmpyacc_noshuff_u8(unsigned short *acc, const unsigned char *a,
                                    unsigned char b, unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_u8
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8_noshuff

void check_poly_vmpyacc_noshuff_u16(unsigned int *acc, const unsigned short *a,
                                     unsigned short b, unsigned int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_u16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_u32_vmpyacc_u32u16u16_noshuff

void check_poly_vmpyacc_noshuff_i16(int *acc, const short *a, short b,
                                     int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_noshuff

// char scalar -> u16 acc (u16u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpyacc_noshuff_char(char *acc, const char *a, char b,
                                      unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8_noshuff

// char scalar, short acc, unsigned char vector -> u16 output (u8u8 variant, char is unsigned on Hexagon)
void check_poly_vmpyacc_noshuff_i16acc_u8_char(short *acc,
                                                const unsigned char *a, char b,
                                                unsigned short *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_noshuff_i16acc_u8_char
// CHECK: call <128 x i16> @ripple_ew_pure_hvx_u16_vmpyacc_u16u8u8_noshuff

// --- hvx_vmpyacc_sat ---

void check_poly_vmpyacc_sat_i16(int *acc, const short *a, short b, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_sat(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_sat_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat

// --- hvx_vmpyacc_sat_noshuff ---

void check_poly_vmpyacc_sat_noshuff_i16(int *acc, const short *a, short b,
                                         int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 64);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyacc_sat_noshuff(acc[v], a[v], b);
}
// CHECK: @check_poly_vmpyacc_sat_noshuff_i16
// CHECK: call <64 x i32> @ripple_ew_pure_hvx_i32_vmpyacc_i32i16i16_sat_noshuff

} // extern "C"
