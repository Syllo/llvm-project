/*==============================================================================
 * Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 * =============================================================================
 *
 * vi64_t.h — Vertical 64-bit integer type for HVX via Ripple hvx_vi64_t
 *
 * Represents a vector of 32 × int64 values as two 32-element int32 planes
 * (lo and hi). This avoids HVX vector-pair register pressure by keeping
 * each plane in a single HVX vector register.
 *
 * The 64-bit add/sub uses hardware carry-chain instructions.
 * The widening 32×32→64 multiply uses vmpye/vmpyoacc.
 *
 * Usage (inside a Ripple SPMD region with ripple_id):
 *   hvx_vi64_t acc = hvx_vi64_zero();
 *   acc = hvx_vi64_add(acc, hvx_vi64_mul32(a, b));
 *   int64 result = hvx_vi64_to_i64(acc);   // only at final store
 *
 * In C++, the +, - and >> operators are also available.
 *
 *============================================================================*/

#ifndef HVX_VI64_H
#define HVX_VI64_H

#if !defined(__hexagon__) || !defined(__HVX__) || !defined(__RIPPLE__)
#error "vi64_t.h requires Hexagon/HVX and Ripple (-fenable-ripple)"
#endif

#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <ripple.h>
#include <ripple_hvx.h>

#ifdef __cplusplus
#define vi64_r2v(n, t, b, x) ripple_to_vec<n, t>(b, x)
#define vi64_v2r(n, t, b, x) vec_to_ripple<n, t>(b, x)
#else
#define vi64_r2v(n, t, b, x) ripple_to_vec(n, t, b, x)
#define vi64_v2r(n, t, b, x) vec_to_ripple(n, t, b, x)
#endif

#ifndef VI64_HVX_PE
#define WE_DEFINED_VI64_HVX_PE 1
#define VI64_HVX_PE 0
#endif

/*
 * The vertical int64 type: two int32 values representing lo and hi halves.
 * In a Ripple SPMD context with block shape (32), each int is a shaped
 * Tensor[32], so the pair represents 32 × int64 in two HVX vector registers.
 */
typedef struct {
  long lo;
  long hi;
} hvx_vi64_t;

/* Zero constant */
static inline __attribute__((always_inline)) hvx_vi64_t hvx_vi64_zero(void) {
  hvx_vi64_t r;
  r.lo = 0;
  r.hi = 0;
  return r;
}

/// @brief Construct from lo/hi halves */
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_from_lohi(long lo, long hi) {
  hvx_vi64_t r;
  r.lo = lo;
  r.hi = hi;
  return r;
}

/// @brief 64-bit addition with carry: result = a + b
// Uses HVX carry - chain add : lo =
//  a.lo + b.lo(with carry out) * hi =
// a.hi + b.hi + carry
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_add(hvx_vi64_t a, hvx_vi64_t b) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector a_lo = vi64_r2v(32, long, one_plane, a.lo);
  HVX_Vector a_hi = vi64_r2v(32, long, one_plane, a.hi);
  HVX_Vector b_lo = vi64_r2v(32, long, one_plane, b.lo);
  HVX_Vector b_hi = vi64_r2v(32, long, one_plane, b.hi);

  HVX_VectorPred carry = Q6_Q_vsetq_R(0x00);
  HVX_Vector sum_lo = Q6_Vw_vadd_VwVwQ_carry(a_lo, b_lo, &carry);
  HVX_Vector sum_hi = Q6_Vw_vadd_VwVwQ_carry(a_hi, b_hi, &carry);

  hvx_vi64_t r;
  r.lo = vi64_v2r(32, long, one_plane, sum_lo);
  r.hi = vi64_v2r(32, long, one_plane, sum_hi);
  return r;
}

/// @brief 64-bit subtraction with borrow: result = a - b
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_sub(hvx_vi64_t a, hvx_vi64_t b) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector a_lo = vi64_r2v(32, long, one_plane, a.lo);
  HVX_Vector a_hi = vi64_r2v(32, long, one_plane, a.hi);
  HVX_Vector b_lo = vi64_r2v(32, long, one_plane, b.lo);
  HVX_Vector b_hi = vi64_r2v(32, long, one_plane, b.hi);

  HVX_VectorPred borrow = Q6_Q_vsetq_R(0xFF);
  HVX_Vector dif_lo = Q6_Vw_vsub_VwVwQ_carry(a_lo, b_lo, &borrow);
  HVX_Vector dif_hi = Q6_Vw_vsub_VwVwQ_carry(a_hi, b_hi, &borrow);

  hvx_vi64_t r;
  r.lo = vi64_v2r(32, long, one_plane, dif_lo);
  r.hi = vi64_v2r(32, long, one_plane, dif_hi);
  return r;
}

/// @brief Widening signed 32×32→64 multiply: result = (int64)a * (int64)b
// Uses vmpye(even / unsigned - half) +
//    vmpyoacc(odd / signed - half).
static inline __attribute__((always_inline)) hvx_vi64_t hvx_vi64_mul32(long a,
                                                                       long b) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector va = vi64_r2v(32, long, one_plane, a);
  HVX_Vector vb = vi64_r2v(32, long, one_plane, b);

  /* vmpye: Vw * Vuh → Vd (signed × unsigned-half, even elements) */
  HVX_VectorPair prod = Q6_W_vmpye_VwVuh(va, vb);
  /* vmpyoacc: accumulate signed × signed-half (odd elements) */
  prod = Q6_W_vmpyoacc_WVwVh(prod, va, vb);

  /* prod is a vector pair: lo=even result, hi=odd result.
   * For signed 32×32→64, lo = lower 32 bits, hi = upper 32 bits. */
  hvx_vi64_t r;
  r.lo = vi64_v2r(32, long, one_plane, Q6_V_lo_W(prod));
  r.hi = vi64_v2r(32, long, one_plane, Q6_V_hi_W(prod));
  return r;
}

/// @brief Converts vertical int64 back to native/flat/horizontal int64. */
static inline __attribute__((always_inline)) long long
hvx_vi64_to_i64(hvx_vi64_t v) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector hiV = vi64_r2v(32, long, one_plane, v.hi);
  HVX_Vector loV = vi64_r2v(32, long, one_plane, v.lo);
  v32i64 flat = (v32i64)Q6_W_vshuff_VVR(hiV, loV, -4);
  return vi64_v2r(32, long long, one_plane, flat);
}

/// @brief Converts a block of 32 flat int64s to 32 vertical int64.
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_from_i64(long long flat) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  v32i64 v = vi64_r2v(32, long long, one_plane, flat);
  HVX_Vector loV = Q6_V_lo_W(v);
  HVX_Vector hiV = Q6_V_hi_W(v);
  HVX_VectorPair flatV = Q6_W_vdeal_VVR(hiV, loV, -4);
  HVX_Vector flatLoV = Q6_V_lo_W(flatV);
  HVX_Vector flatHiV = Q6_V_hi_W(flatV);

  long lo = vi64_v2r(32, long, one_plane, flatLoV);
  long hi = vi64_v2r(32, long, one_plane, flatHiV);
  return hvx_vi64_from_lohi(lo, hi);
}

/// @brief Vector arithmetic right shift, with shift in [0, 31].
// TODO: extend to [0, 63] shifts
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_asr_31(hvx_vi64_t a, long shift) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  // With this bcast, we support both cases where shift is a scalar or a vector
  // If it's a vector, the broadcast is a nop.
  // long shiftB = ripple_broadcast(one_plane, 0x1, shift);
  HVX_Vector shiftV = vi64_r2v(32, long, one_plane, shift);
  HVX_Vector aLoV = vi64_r2v(32, long, one_plane, a.lo);
  HVX_Vector rotLoV = Q6_Vuw_vrotr_VuwVuw(aLoV, shiftV);
  HVX_Vector aHiV = vi64_r2v(32, long, one_plane, a.hi);
  HVX_VectorPair halfRotated = Q6_W_vcombine_VV(aHiV, rotLoV);
  HVX_VectorPair valV = Q6_Ww_vasrinto_WwVwVw(halfRotated, aHiV, shiftV);
  HVX_Vector valV_lo = Q6_V_lo_W(valV);
  HVX_Vector valV_hi = Q6_V_hi_W(valV);
  long val_lo = vi64_v2r(32, long, one_plane, valV_lo);
  long val_hi = vi64_v2r(32, long, one_plane, valV_hi);
  return hvx_vi64_from_lohi(val_lo, val_hi);
}

/* ===== Fused multiply-accumulate / multiply-subtract ===== */

///  @brief Fused multiply-accumulate: acc += (int64)a * (int64)b
/// Product stays as a temporary vector pair (never stored).
// 4 HVX instructions: vmpye, vmpyoacc, vadd_carry(lo), vadd_carry(hi).
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_mac(hvx_vi64_t acc, long a, long b) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector va = vi64_r2v(32, long, one_plane, a);
  HVX_Vector vb = vi64_r2v(32, long, one_plane, b);
  HVX_VectorPair prod = Q6_W_vmpye_VwVuh(va, vb);
  prod = Q6_W_vmpyoacc_WVwVh(prod, va, vb);

  HVX_Vector acc_lo = vi64_r2v(32, long, one_plane, acc.lo);
  HVX_Vector acc_hi = vi64_r2v(32, long, one_plane, acc.hi);
  HVX_VectorPred carry = Q6_Q_vsetq_R(0x00);
  acc_lo = Q6_Vw_vadd_VwVwQ_carry(acc_lo, Q6_V_lo_W(prod), &carry);
  acc_hi = Q6_Vw_vadd_VwVwQ_carry(acc_hi, Q6_V_hi_W(prod), &carry);

  hvx_vi64_t r;
  r.lo = vi64_v2r(32, long, one_plane, acc_lo);
  r.hi = vi64_v2r(32, long, one_plane, acc_hi);
  return r;
}

/// @brief Fused multiply-subtract: acc -= (int64)a * (int64)b
// 4 HVX instructions: vmpye, vmpyoacc, vsub_carry(lo), vsub_carry(hi).
static inline __attribute__((always_inline)) hvx_vi64_t
hvx_vi64_msc(hvx_vi64_t acc, long a, long b) {
  ripple_block_t one_plane = ripple_set_block_shape(VI64_HVX_PE, 32);
  HVX_Vector va = vi64_r2v(32, long, one_plane, a);
  HVX_Vector vb = vi64_r2v(32, long, one_plane, b);
  HVX_VectorPair prod = Q6_W_vmpye_VwVuh(va, vb);
  prod = Q6_W_vmpyoacc_WVwVh(prod, va, vb);

  HVX_Vector acc_lo = vi64_r2v(32, long, one_plane, acc.lo);
  HVX_Vector acc_hi = vi64_r2v(32, long, one_plane, acc.hi);
  HVX_VectorPred borrow = Q6_Q_vsetq_R(0xFF);
  acc_lo = Q6_Vw_vsub_VwVwQ_carry(acc_lo, Q6_V_lo_W(prod), &borrow);
  acc_hi = Q6_Vw_vsub_VwVwQ_carry(acc_hi, Q6_V_hi_W(prod), &borrow);

  hvx_vi64_t r;
  r.lo = vi64_v2r(32, long, one_plane, acc_lo);
  r.hi = vi64_v2r(32, long, one_plane, acc_hi);
  return r;
}

#undef vi64_v2r
#undef vi64_r2v
#ifdef WE_DEFINED_VI64_HVX_PE
#undef WE_DEFINED_VI64_HVX_PE
#undef VI64_HVX_PE
#endif

#ifdef __cplusplus

static inline __attribute__((always_inline)) hvx_vi64_t
operator+(hvx_vi64_t a, hvx_vi64_t b) {
  return hvx_vi64_add(a, b);
}

static inline __attribute__((always_inline)) hvx_vi64_t
operator-(hvx_vi64_t a, hvx_vi64_t b) {
  return hvx_vi64_sub(a, b);
}

/// @brief right shift on hvx_vi64_t.
/// @param shift Currently needs to be in [0, 31]. For forward-compatible code
/// assuming shifts in [0, 31], please use hvx_vi64_asr_31.
static inline __attribute__((always_inline)) hvx_vi64_t operator>>(hvx_vi64_t a,
                                                                   long shift) {
  return hvx_vi64_asr_31(a, shift);
}

static inline __attribute__((always_inline)) hvx_vi64_t &
operator+=(hvx_vi64_t &a, hvx_vi64_t b) {
  a = a + b;
  return a;
}

static inline __attribute__((always_inline)) hvx_vi64_t &
operator-=(hvx_vi64_t &a, hvx_vi64_t b) {
  a = a - b;
  return a;
}

/// @brief in-place right shift on hvx_vi64_t.
/// @param shift Currently needs to be in [0, 31]. For forward-compatible code
/// assuming shifts in [0, 31], please use hvx_vi64_asr_31.
static inline __attribute__((always_inline)) hvx_vi64_t &
operator>>=(hvx_vi64_t &a, long shift) {
  a = a >> shift;
  return a;
}
#endif

#endif /* HVX_VI64_H */