// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mv81 -emit-llvm %s -o - 2>&1 | FileCheck %s

// This test verifies that the Ripple compiler pass correctly replaces calls to
// the hvx_i32_vmpyo_* API functions (declared in HVX_Vmpy_x32x16.h) with the
// corresponding ripple_ew_pure_hvx_* implementations.
//
// Section 1: typed names (hvx_i32_vmpyo_i32i16_s1_rnd_sat, etc.)
// Section 2: polymorphic names (hvx_vmpyo_s1_rnd_sat, hvx_vmpyo_s1_sat)
//
// Each test function uses ripple_id to index directly into the arrays so the
// Ripple pass can recognize and vectorize the call.  The block shape is set to
// 32 elements (one 128-byte HVX i32 vector).
//
// The FileCheck lines verify the expected @ripple_ew_pure_hvx_* symbol appears
// in the emitted LLVM IR.

#include "ripple_test.h"
// HVX_Vmpy_x32x16 RT library declarations, inlined so this frontend test does
// not depend on the compiler-rt headers or the Hexagon SDK.
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
#include "ripple_hvx_test_types.h"

static_assert(sizeof(int) == 4, "HVX_Vmpy_x32x16: unexpected int size");

#ifdef __cplusplus
extern "C" {
#endif

#define _decl_hvx_vmpyo_s1_rnd_sat(cType, type, cScType, scType)               \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpyo_##type##scType##_s1_rnd_sat(cType m, cScType mc)

#define _decl_hvx_vmpyo_s1_sat(cType, type, cScType, scType)                   \
  __attribute__((always_inline)) extern cType                                  \
      hvx_##type##_vmpyo_##type##scType##_s1_sat(cType m, cScType mc)

_decl_hvx_vmpyo_s1_rnd_sat(int, i32, int,
                           i16);
_decl_hvx_vmpyo_s1_sat(int, i32, int, i16);

#undef _decl_hvx_vmpyo_s1_rnd_sat
#undef _decl_hvx_vmpyo_s1_sat

extern RIPPLE_VECTOR_CANDIDATE v32i32
ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_rnd_sat(v32i32 m, v32i32 mc);
extern RIPPLE_VECTOR_CANDIDATE v32i32
ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_sat(v32i32 m, v32i32 mc);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

#define hvx_vmpyo_s1_rnd_sat(m, mc)                                            \
  _Generic((m), int: hvx_i32_vmpyo_i32i16_s1_rnd_sat((m), (mc)))

#define hvx_vmpyo_s1_sat(m, mc)                                                \
  _Generic((m), int: hvx_i32_vmpyo_i32i16_s1_sat((m), (mc)))

#else

[[gnu::always_inline, gnu::unused]] static int hvx_vmpyo_s1_rnd_sat(int m,
                                                                    int mc) {
  return hvx_i32_vmpyo_i32i16_s1_rnd_sat(m, mc);
}

[[gnu::always_inline, gnu::unused]] static int hvx_vmpyo_s1_sat(int m, int mc) {
  return hvx_i32_vmpyo_i32i16_s1_sat(m, mc);
}

#endif

extern "C" {

// =============================================================================
// Section 1: typed API
// =============================================================================

void check_vmpyo_i32i16_s1_rnd_sat(size_t len, const int *m, const int *mc,
                                    int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyo_i32i16_s1_rnd_sat(m[v], mc[v]);
}
// CHECK: @check_vmpyo_i32i16_s1_rnd_sat
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_rnd_sat

void check_vmpyo_i32i16_s1_sat(size_t len, const int *m, const int *mc,
                                int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  int v = ripple_id(BS, 0);
  out[v] = hvx_i32_vmpyo_i32i16_s1_sat(m[v], mc[v]);
}
// CHECK: @check_vmpyo_i32i16_s1_sat
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_sat

// =============================================================================
// Section 2: polymorphic API (hvx_vmpyo_s1_rnd_sat / hvx_vmpyo_s1_sat)
// =============================================================================
// Only int (i32) is supported; dispatch is trivial.

void check_poly_vmpyo_s1_rnd_sat(const int *m, const int *mc, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyo_s1_rnd_sat(m[v], mc[v]);
}
// CHECK: @check_poly_vmpyo_s1_rnd_sat
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_rnd_sat

void check_poly_vmpyo_s1_sat(const int *m, const int *mc, int *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  int v = ripple_id(BS, 0);
  out[v] = hvx_vmpyo_s1_sat(m[v], mc[v]);
}
// CHECK: @check_poly_vmpyo_s1_sat
// CHECK: call <32 x i32> @ripple_ew_pure_hvx_i32_vmpyo_i32i16_s1_sat

} // extern "C"
