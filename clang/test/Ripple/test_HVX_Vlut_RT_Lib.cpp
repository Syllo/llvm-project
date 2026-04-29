// UNSUPPORTED: cui
// REQUIRES: hexagon-registered-target
// REQUIRES: rt_lib
// RUN: %clang++ -g -S -fenable-ripple --target=hexagon -mhvx -mhvx-length=128B -mv85 -emit-llvm %s -o - -mllvm -ripple-disable-link 2>&1 | FileCheck %s

// This test verifies that the Ripple compiler pass replaces calls to the
// hvx_prepare_lut_* / hvx_vlut_* entry points (declared in HVX_Vlut.h) with the
// corresponding ripple_* runtime implementations.  Each check function sets up
// an in-lane lookup for a given table size and element width, and the
// FileCheck lines confirm the expected ripple_* symbols appear in the emitted
// LLVM IR.

#include <ripple.h>
#include <ripple/HVX_Vlut.h>

extern "C" {

void check_lut_u8_256_w_stack() {
  ripple_block_t BS = ripple_set_block_shape(0, 256);
  int v = ripple_id(BS, 0); // 0-255
  unsigned char table[256], indices[128], out[128];
  unsigned char prepared_lut[256];
  prepared_lut[v] = hvx_prepare_lut_256u8(table[v]);
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  ripple_parallel(BS128, 0);
  for (int i = 0; i < 128; i++)
    out[i] = hvx_vlut_u8_w_ptr(indices[i], prepared_lut);
}
// CHECK-LABEL: @check_lut_u8_256_w_stack
// CHECK: call {{.*}}<256 x i8> @ripple_hvx_prepare_lut_256u8
// CHECK: call {{.*}}<128 x i8> @ripple_ew_pure_hvx_vlut_u8_w_ptr

void check_lut_u8_256_w_value() {
  ripple_block_t BS = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS, 0); // 0-127
  int v1 = ripple_id(BS, 1); // 0-1
  unsigned char table[256], indices[128], out[128];
  unsigned char prepared_lut = hvx_prepare_lut_128x2u8(table[v1*128+v0]);
  out[v0] = hvx_vlut_u8_w_value(indices[v0], prepared_lut);
}
// CHECK-LABEL: @check_lut_u8_256_w_value
// CHECK: call {{.*}}<256 x i8> @ripple_arg0_t128x2u8_ret_t128x2u8_hvx_prepare_lut_128x2u8
// CHECK: call {{.*}}<128 x i8> @ripple_pure_arg0_t128u8_arg1_t128x2u8_hvx_vlut_u8_w_value

void check_lut_u8_128_w_stack() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS128, 0);
  int nv = ripple_get_block_size(BS128, 0);
  unsigned char table[256], out[128], indices[128], prepared_lut[256];
  prepared_lut[v] = hvx_prepare_lut_128u8(table[v]);       // entries 0-127
  prepared_lut[nv + v] = hvx_prepare_lut_128u8(table[nv + v]); // entries 128-255
  ripple_parallel(BS128, 0);
  for (int i = 0; i < 128; i++)
    out[i] = hvx_vlut_u8_w_ptr(indices[i], prepared_lut);
}
// CHECK-LABEL: @check_lut_u8_128_w_stack
// CHECK: call {{.*}}<128 x i8> @ripple_hvx_prepare_lut_128u8
// CHECK: call {{.*}}<128 x i8> @ripple_ew_pure_hvx_vlut_u8_w_ptr

void check_lut_u8_128_w_value() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v = ripple_id(BS128, 0);
  int nv = ripple_get_block_size(BS128, 0);
  unsigned char table[256], out[128], indices[128], prepared_lo, prepared_hi;
  prepared_lo = hvx_prepare_lut_128u8(table[v]);       // entries 0-127
  prepared_hi = hvx_prepare_lut_128u8(table[nv + v]); // entries 128-255
  ripple_parallel(BS128, 0);
  for (int i = 0; i < 128; i++)
    out[i] = hvx_vlut_u8_2vec(indices[i], prepared_lo, prepared_hi);
}
// CHECK-LABEL: @check_lut_u8_128_w_value
// CHECK: call {{.*}}<128 x i8> @ripple_hvx_prepare_lut_128u8
// CHECK: call {{.*}}<128 x i8> @ripple_ew_pure_hvx_vlut_u8_2vec

void check_vlut_64u16() {
  ripple_block_t BS64x2 = ripple_set_block_shape(0, 64, 2);
  int v0 = ripple_id(BS64x2, 0); // 0-63
  int v1 = ripple_id(BS64x2, 1); // 0-1
  unsigned short table[64], out[128];
  unsigned char indices[128];
  unsigned short prepared = ripple_slice(out[v1*64+v0], -1, 0); // 64 x 1 u16
  prepared = hvx_prepare_lut_64u16(table[v0]);
  out[v1*64+v0] = hvx_vlut_64u16(indices[v1*64+v0], prepared);
}
// CHECK-LABEL: @check_vlut_64u16
// CHECK: call {{.*}}@ripple_arg0_t64u16_ret_t64u16_hvx_prepare_lut_64u16
// CHECK: call {{.*}}@ripple_pure_arg0_t64x2u8_arg1_t64u16_ret_t64x2u16_hvx_vlut_64u16

void check_vlut_128u16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  unsigned short table[128], out[128];
  unsigned char indices[128];
  unsigned short prepared = hvx_prepare_lut_128u16(table[v0]);
  out[v0] = hvx_vlut_128u16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128u16
// CHECK: call {{.*}}@ripple_arg0_t128u16_ret_t128u16_hvx_prepare_lut_128u16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u16_ret_t128u16_hvx_vlut_128u16

void check_vlut_256u16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  unsigned short table[256], out[128];
  unsigned char indices[128];
  unsigned short prepared = hvx_prepare_lut_256u16(table[v1*128+v0]);
  out[v0] = hvx_vlut_256u16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_256u16
// CHECK: call {{.*}}@ripple_arg0_t128x2u16_ret_t128x2u16_hvx_prepare_lut_256u16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2u16_ret_t128u16_hvx_vlut_256u16

void check_vlut_128u32() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  unsigned int table[128], out[128];
  unsigned char indices[128];
  unsigned int prepared = hvx_prepare_lut_128u32(table[v0]);
  out[v0] = hvx_vlut_128u32(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128u32
// CHECK: call {{.*}}@ripple_arg0_t128u32_ret_t128u32_hvx_prepare_lut_128u32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u32_ret_t128u32_hvx_vlut_128u32

void check_vlut_256u32() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  unsigned int table[256], out[128];
  unsigned char indices[128];
  unsigned int prepared = hvx_prepare_lut_256u32(table[v1*128+v0]);
  out[v0] = hvx_vlut_256u32(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_256u32
// CHECK: call {{.*}}@ripple_arg0_t128x2u32_ret_t128x2u32_hvx_prepare_lut_256u32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2u32_ret_t128u32_hvx_vlut_256u32

void check_vlut_64u64() {
  ripple_block_t BS64x2 = ripple_set_block_shape(0, 64, 2);
  int v0 = ripple_id(BS64x2, 0); // 0-63
  int v1 = ripple_id(BS64x2, 1); // 0-1
  unsigned long long table[64], out[128];
  unsigned char indices[128];
  unsigned long long prepared = hvx_prepare_lut_64u64(table[v0]);
  out[v1*64+v0] = hvx_vlut_64u64(indices[v1*64+v0], prepared);
}
// CHECK-LABEL: @check_vlut_64u64
// CHECK: call {{.*}}@ripple_arg0_t64u64_ret_t64u64_hvx_prepare_lut_64u64
// CHECK: call {{.*}}@ripple_pure_arg0_t64x2u8_arg1_t64u64_ret_t64x2u64_hvx_vlut_64u64

void check_vlut_128u64() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  unsigned long long table[128], out[128];
  unsigned char indices[128];
  unsigned long long prepared = hvx_prepare_lut_128u64(table[v0]);
  out[v0] = hvx_vlut_128u64(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128u64
// CHECK: call {{.*}}@ripple_arg0_t128u64_ret_t128u64_hvx_prepare_lut_128u64
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u64_ret_t128u64_hvx_vlut_128u64

// --- Polymorphic hvx_prepare_lut / hvx_vlut wrappers ---
// These verify the generic entry points resolve to the natural single-block
// implementation for each element type (u16->128u16, u32->128u32, u64->64u64).
// All three resolve to 1-D block sizes, so a 1-D ripple_set_block_shape is
// used (no 2-D shape needed): the 2-D table layout is only required by the
// double-length 256u16/256u32/128u64 variants accessed via their explicit
// names.

void check_vlut_poly_u16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  unsigned short table[128], out[128];
  unsigned char indices[128];
  unsigned short prepared = hvx_prepare_lut(table[v0]);
  out[v0] = hvx_vlut(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_u16
// CHECK: call {{.*}}@ripple_arg0_t128u16_ret_t128u16_hvx_prepare_lut_128u16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u16_ret_t128u16_hvx_vlut_128u16

void check_vlut_poly_u32() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  unsigned int table[128], out[128];
  unsigned char indices[128];
  unsigned int prepared = hvx_prepare_lut(table[v0]);
  out[v0] = hvx_vlut(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_u32
// CHECK: call {{.*}}@ripple_arg0_t128u32_ret_t128u32_hvx_prepare_lut_128u32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u32_ret_t128u32_hvx_vlut_128u32

void check_vlut_poly_u64() {
  ripple_block_t BS64x2 = ripple_set_block_shape(0, 64, 2);
  int v0 = ripple_id(BS64x2, 0); // 0-63
  int v1 = ripple_id(BS64x2, 1); // 0-1
  unsigned long long table[64], out[128];
  unsigned char indices[128];
  unsigned long long prepared = hvx_prepare_lut(table[v0]);
  out[v1*64+v0] = hvx_vlut(indices[v1*64+v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_u64
// CHECK: call {{.*}}@ripple_arg0_t64u64_ret_t64u64_hvx_prepare_lut_64u64
// CHECK: call {{.*}}@ripple_pure_arg0_t64x2u8_arg1_t64u64_ret_t64x2u64_hvx_vlut_64u64

// --- Polymorphic double-vector-length hvx_prepare_lut_double / hvx_vlut_double ---
// These verify the double-length generic entry points resolve to the 2-D
// (double vector length) implementation for each element type
// (u16->256u16, u32->256u32, u64->128u64).  These require a 2-D block shape
// because the table holds twice the single-block capacity.

void check_vlut_poly_double_u16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  unsigned short table[256], out[128];
  unsigned char indices[128];
  unsigned short prepared = hvx_prepare_lut_double(table[v1*128+v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_u16
// CHECK: call {{.*}}@ripple_arg0_t128x2u16_ret_t128x2u16_hvx_prepare_lut_256u16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2u16_ret_t128u16_hvx_vlut_256u16

void check_vlut_poly_double_u32() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  unsigned int table[256], out[128];
  unsigned char indices[128];
  unsigned int prepared = hvx_prepare_lut_double(table[v1*128+v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_u32
// CHECK: call {{.*}}@ripple_arg0_t128x2u32_ret_t128x2u32_hvx_prepare_lut_256u32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2u32_ret_t128u32_hvx_vlut_256u32

void check_vlut_poly_double_u64() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-63
  unsigned long long table[128], out[128];
  unsigned char indices[128];
  unsigned long long prepared = hvx_prepare_lut_double(table[v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_u64
// CHECK: call {{.*}}@ripple_arg0_t128u64_ret_t128u64_hvx_prepare_lut_128u64
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128u64_ret_t128u64_hvx_vlut_128u64

// --- Floating-point lookups ---
// f32 lookups reuse the u32 runtime (float reinterpreted as u32); f64 lookups
// reuse the u64 runtime (double reinterpreted as u64).

void check_vlut_128f32() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  float table[128], out[128];
  unsigned char indices[128];
  float prepared = hvx_prepare_lut_128f32(table[v0]);
  out[v0] = hvx_vlut_128f32(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128f32
// CHECK: call {{.*}}@ripple_arg0_t128f32_ret_t128f32_hvx_prepare_lut_128f32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f32_ret_t128f32_hvx_vlut_128f32

void check_vlut_256f32() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  float table[256], out[128];
  unsigned char indices[128];
  float prepared = hvx_prepare_lut_256f32(table[v1*128+v0]);
  out[v0] = hvx_vlut_256f32(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_256f32
// CHECK: call {{.*}}@ripple_arg0_t128x2f32_ret_t128x2f32_hvx_prepare_lut_256f32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2f32_ret_t128f32_hvx_vlut_256f32

void check_vlut_64f64() {
  ripple_block_t BS64x2 = ripple_set_block_shape(0, 64, 2);
  int v0 = ripple_id(BS64x2, 0); // 0-63
  int v1 = ripple_id(BS64x2, 1); // 0-1
  double table[64], out[128];
  unsigned char indices[128];
  double prepared = hvx_prepare_lut_64f64(table[v0]);
  out[v1*64+v0] = hvx_vlut_64f64(indices[v1*64+v0], prepared);
}
// CHECK-LABEL: @check_vlut_64f64
// CHECK: call {{.*}}@ripple_arg0_t64f64_ret_t64f64_hvx_prepare_lut_64f64
// CHECK: call {{.*}}@ripple_pure_arg0_t64x2u8_arg1_t64f64_ret_t64x2f64_hvx_vlut_64f64

void check_vlut_128f64() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  double table[128], out[128];
  unsigned char indices[128];
  double prepared = hvx_prepare_lut_128f64(table[v0]);
  out[v0] = hvx_vlut_128f64(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128f64
// CHECK: call {{.*}}@ripple_arg0_t128f64_ret_t128f64_hvx_prepare_lut_128f64
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f64_ret_t128f64_hvx_vlut_128f64

// --- Polymorphic float/double dispatch ---
// hvx_prepare_lut / hvx_vlut dispatch float -> 128f32 and double -> 64f64.

void check_vlut_poly_f32() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  float table[128], out[128];
  unsigned char indices[128];
  float prepared = hvx_prepare_lut(table[v0]);
  out[v0] = hvx_vlut(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_f32
// CHECK: call {{.*}}@ripple_arg0_t128f32_ret_t128f32_hvx_prepare_lut_128f32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f32_ret_t128f32_hvx_vlut_128f32

void check_vlut_poly_f64() {
  ripple_block_t BS64x2 = ripple_set_block_shape(0, 64, 2);
  int v0 = ripple_id(BS64x2, 0); // 0-63
  int v1 = ripple_id(BS64x2, 1); // 0-1
  double table[64], out[128];
  unsigned char indices[128];
  double prepared = hvx_prepare_lut(table[v0]);
  out[v1*64+v0] = hvx_vlut(indices[v1*64+v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_f64
// CHECK: call {{.*}}@ripple_arg0_t64f64_ret_t64f64_hvx_prepare_lut_64f64
// CHECK: call {{.*}}@ripple_pure_arg0_t64x2u8_arg1_t64f64_ret_t64x2f64_hvx_vlut_64f64

// hvx_prepare_lut_double / hvx_vlut_double dispatch float -> 256f32 (prepare) +
// 128f32 (vlut) and double -> 128f64.

void check_vlut_poly_double_f32() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  float table[256], out[128];
  unsigned char indices[128];
  float prepared = hvx_prepare_lut_double(table[v1*128+v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_f32
// CHECK: call {{.*}}@ripple_arg0_t128x2f32_ret_t128x2f32_hvx_prepare_lut_256f32
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2f32_ret_t128f32_hvx_vlut_256f32

void check_vlut_poly_double_f64() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  double table[128], out[128];
  unsigned char indices[128];
  double prepared = hvx_prepare_lut_double(table[v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_f64
// CHECK: call {{.*}}@ripple_arg0_t128f64_ret_t128f64_hvx_prepare_lut_128f64
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f64_ret_t128f64_hvx_vlut_128f64

// --- 16-bit floating-point lookups ---
// f16/bf16 lookups reuse the u16 runtime (the 16-bit float reinterpreted as
// u16).

#if __has_Float16__
void check_vlut_128f16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  _Float16 table[128], out[128];
  unsigned char indices[128];
  _Float16 prepared = hvx_prepare_lut_128f16(table[v0]);
  out[v0] = hvx_vlut_128f16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128f16
// CHECK: call {{.*}}@ripple_arg0_t128f16_ret_t128f16_hvx_prepare_lut_128f16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f16_ret_t128f16_hvx_vlut_128f16

void check_vlut_256f16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  _Float16 table[256], out[128];
  unsigned char indices[128];
  _Float16 prepared = hvx_prepare_lut_256f16(table[v1*128+v0]);
  out[v0] = hvx_vlut_256f16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_256f16
// CHECK: call {{.*}}@ripple_arg0_t128x2f16_ret_t128x2f16_hvx_prepare_lut_256f16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2f16_ret_t128f16_hvx_vlut_256f16
#endif // __has_Float16__

#if __has_bf16__
void check_vlut_128bf16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  __bf16 table[128], out[128];
  unsigned char indices[128];
  __bf16 prepared = hvx_prepare_lut_128bf16(table[v0]);
  out[v0] = hvx_vlut_128bf16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_128bf16
// CHECK: call {{.*}}@ripple_arg0_t128bf16_ret_t128bf16_hvx_prepare_lut_128bf16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128bf16_ret_t128bf16_hvx_vlut_128bf16

void check_vlut_256bf16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  __bf16 table[256], out[128];
  unsigned char indices[128];
  __bf16 prepared = hvx_prepare_lut_256bf16(table[v1*128+v0]);
  out[v0] = hvx_vlut_256bf16(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_256bf16
// CHECK: call {{.*}}@ripple_arg0_t128x2bf16_ret_t128x2bf16_hvx_prepare_lut_256bf16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2bf16_ret_t128bf16_hvx_vlut_256bf16
#endif // __has_bf16__

// --- Polymorphic f16/bf16 dispatch ---
// hvx_prepare_lut / hvx_vlut dispatch _Float16 -> 128f16 and __bf16 -> 128bf16.

#if __has_Float16__
void check_vlut_poly_f16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  _Float16 table[128], out[128];
  unsigned char indices[128];
  _Float16 prepared = hvx_prepare_lut(table[v0]);
  out[v0] = hvx_vlut(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_f16
// CHECK: call {{.*}}@ripple_arg0_t128f16_ret_t128f16_hvx_prepare_lut_128f16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128f16_ret_t128f16_hvx_vlut_128f16
#endif // __has_Float16__

#if __has_bf16__
void check_vlut_poly_bf16() {
  ripple_block_t BS128 = ripple_set_block_shape(0, 128);
  int v0 = ripple_id(BS128, 0); // 0-127
  __bf16 table[128], out[128];
  unsigned char indices[128];
  __bf16 prepared = hvx_prepare_lut(table[v0]);
  out[v0] = hvx_vlut(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_bf16
// CHECK: call {{.*}}@ripple_arg0_t128bf16_ret_t128bf16_hvx_prepare_lut_128bf16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128bf16_ret_t128bf16_hvx_vlut_128bf16
#endif // __has_bf16__

// hvx_prepare_lut_double / hvx_vlut_double dispatch _Float16 -> 256f16 (prepare)
// + 128f16 (vlut) and __bf16 -> 256bf16 (prepare) + 128bf16 (vlut).

#if __has_Float16__
void check_vlut_poly_double_f16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  _Float16 table[256], out[128];
  unsigned char indices[128];
  _Float16 prepared = hvx_prepare_lut_double(table[v1*128+v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_f16
// CHECK: call {{.*}}@ripple_arg0_t128x2f16_ret_t128x2f16_hvx_prepare_lut_256f16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2f16_ret_t128f16_hvx_vlut_256f16
#endif // __has_Float16__

#if __has_bf16__
void check_vlut_poly_double_bf16() {
  ripple_block_t BS128x2 = ripple_set_block_shape(0, 128, 2);
  int v0 = ripple_id(BS128x2, 0); // 0-127
  int v1 = ripple_id(BS128x2, 1); // 0-1
  __bf16 table[256], out[128];
  unsigned char indices[128];
  __bf16 prepared = hvx_prepare_lut_double(table[v1*128+v0]);
  out[v0] = hvx_vlut_double(indices[v0], prepared);
}
// CHECK-LABEL: @check_vlut_poly_double_bf16
// CHECK: call {{.*}}@ripple_arg0_t128x2bf16_ret_t128x2bf16_hvx_prepare_lut_256bf16
// CHECK: call {{.*}}@ripple_pure_arg0_t128u8_arg1_t128x2bf16_ret_t128bf16_hvx_vlut_256bf16
#endif // __has_bf16__

} // extern "C"
