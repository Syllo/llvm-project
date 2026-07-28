//===-------------------------- HVX_Scatter_Gather.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//            Header to access hvx_scatter/hvx_gather from Ripple.
//===------------------------------------------------------------------------===

#pragma once

#ifndef __cplusplus
#error "HVX_Scatter_Gather.h requires C++"
#endif

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <ripple.h>
#include <ripple_hvx.h>
#include <stddef.h>
#include <stdint.h>

// This header hard-codes 128-byte HVX vector assumptions (block shapes, lane
// splits, deal offsets).  The old bitcode build enforced -mhvx-length=128B per
// source; in the header-only model the includer must supply it.
#if defined(__HVX_LENGTH__) && __HVX_LENGTH__ != 128
#error "HVX_Scatter_Gather.h requires -mhvx-length=128B"
#endif

// ___________________________ HVX vector gather _______________________________

#ifdef __cplusplus
extern "C" {
#endif

// Scalar declarations. Ripple candidate names mirror these entry points.

#define _decl_hvx_gather(ct, t)                                                \
  void hvx_gather_##t(ct *dst, const ct *src, int index, size_t region_size)

_decl_hvx_gather(int8_t, i8);
_decl_hvx_gather(uint8_t, u8);
_decl_hvx_gather(int16_t, i16);
_decl_hvx_gather(uint16_t, u16);
_decl_hvx_gather(int32_t, i32);
_decl_hvx_gather(uint32_t, u32);
_decl_hvx_gather(int64_t, i64);
_decl_hvx_gather(uint64_t, u64);

_decl_hvx_gather(float, f32);
_decl_hvx_gather(double, f64);
#if __has_bf16__
_decl_hvx_gather(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather(_Float16, f16);
#endif
#undef _decl_hvx_gather

#define _decl_hvx_gather_16(ct, t)                                             \
  void hvx_gather_##t##_16(ct *dst, const ct *src, int16_t index,              \
                           size_t region_size)

_decl_hvx_gather_16(int8_t, i8);
_decl_hvx_gather_16(uint8_t, u8);

_decl_hvx_gather_16(int16_t, i16);
_decl_hvx_gather_16(uint16_t, u16);

#if __has_bf16__
_decl_hvx_gather_16(__bf16, bf16);
#endif
#if __has_Float16__
_decl_hvx_gather_16(_Float16, f16);
#endif

#undef _decl_hvx_gather_16

#ifndef __cplusplus
#if __has_bf16__
#define __extra_bf16_gather(dst, src, index, region_size)                      \
  , __bf16 : hvx_gather_bf16((dst), (src), (index), (region_size))
#else
#define __extra_bf16_gather(dst, src, index, region_size)
#endif

#if __has_Float16__
#define __extra_f16_gather(dst, src, index, region_size)                       \
  , _Float16 : hvx_gather_f16((dst), (src), (index), (region_size))
#else
#define __extra_f16_gather(dst, src, index, region_size)
#endif

#define hvx_gather(dst, src, index, region_size)                               \
  _Generic((src),                                                              \
      char *: _Generic((index),                                                \
          int16_t: hvx_gather_i8_16((dst), (src), (index), (region_size)),     \
          int32_t: hvx_gather_i8((dst), (src), (index), (region_size))),       \
      int16_t *: _Generic((index),                                             \
          int16_t: hvx_gather_i16_16((dst), (src), (index), (region_size)),    \
          int32_t: hvx_gather_i16((dst), (src), (index), (region_size))),      \
      int32_t *: hvx_gather_i32((dst), (src), (index), (region_size)),         \
      int64_t *: hvx_gather_i64((dst), (src), (index), (region_size)),         \
      uint8_t *: _Generic((index),                                             \
          int16_t: hvx_gather_u8_16((dst), (src), (index), (region_size)),     \
          int32_t: hvx_gather_u8((dst), (src), (index), (region_size))),       \
      uint16_t *: _Generic((index),                                            \
          int16_t: hvx_gather_u16_16((dst), (src), (index), (region_size)),    \
          int32_t: hvx_gather_u16((dst), (src), (index), (region_size))),      \
      uint32_t *: hvx_gather_u32((dst), (src), (index), (region_size)),        \
      uint64_t *: hvx_gather_u64((dst), (src), (index), (region_size)),        \
      float *: hvx_gather_f32((dst), (src), (index), (region_size)),           \
      double *: hvx_gather_f64((dst), (src), (index), (region_size))           \
          __extra_bf16_gather(dst, src, index, region_size)                    \
              __extra_f16_gather(dst, src, index, region_size))

#else // __cplusplus

// The `gnu::unused` attribute on static hvx_gather() and hvx_scatter() C++
// functions is added to avoid compiler warnings for unused static functions if
// this file is installed in a path included with -I (not -isystem or its
// variant).
#define _decl_hvx_gather_spec(ct, t)                                           \
  [[gnu::always_inline, gnu::unused]] static void hvx_gather(                  \
      ct *dst, const ct *src, int index, size_t region_size) {                 \
    hvx_gather_##t(dst, src, index, region_size);                              \
  }

// clang-format off
_decl_hvx_gather_spec(int8_t, i8)
_decl_hvx_gather_spec(uint8_t, u8)
_decl_hvx_gather_spec(int16_t, i16)
_decl_hvx_gather_spec(uint16_t, u16)
_decl_hvx_gather_spec(int32_t, i32)
_decl_hvx_gather_spec(uint32_t, u32)
_decl_hvx_gather_spec(int64_t, i64)
_decl_hvx_gather_spec(uint64_t, u64)

_decl_hvx_gather_spec(float, f32)
_decl_hvx_gather_spec(double, f64)
#if __has_bf16__
_decl_hvx_gather_spec(__bf16, bf16)
#endif
#if __has_Float16__
_decl_hvx_gather_spec(_Float16, f16)
#endif

#undef _decl_hvx_gather_spec
// clang-format on

#define _decl_hvx_gather_16_spec(ct, t)                                        \
  [[gnu::always_inline, gnu::unused]] static void hvx_gather(                  \
      ct *dst, const ct *src, int16_t index, size_t region_size) {             \
    hvx_gather_##t##_16(dst, src, index, region_size);                         \
  }

    // clang-format off
_decl_hvx_gather_16_spec(int16_t, i16)
_decl_hvx_gather_16_spec(uint16_t, u16)

_decl_hvx_gather_16_spec(int8_t, i8)
_decl_hvx_gather_16_spec(uint8_t, u8)

#if __has_bf16__
_decl_hvx_gather_16_spec(__bf16, bf16)
#endif
#if __has_Float16__
_decl_hvx_gather_16_spec(_Float16, f16)
#endif

#undef _decl_hvx_gather_16_spec
// clang-format on

#endif // __cplusplus

// __________________________ HVX vector scatter _______________________________

#define _decl_hvx_scatter(w)                                                   \
  void hvx_scatter_i##w(int##w##_t *dst, int index, int##w##_t src,            \
                        size_t region_size);                                   \
  void hvx_scatter_u##w(uint##w##_t *dst, int index, uint##w##_t src,          \
                        size_t region_size)

#define _decl_hvx_scatter_16(w)                                                \
  void hvx_scatter_i##w##_16(int##w##_t *dst, int16_t index, int##w##_t src,   \
                             size_t region_size);                              \
  void hvx_scatter_u##w##_16(uint##w##_t *dst, int16_t index, uint##w##_t src, \
                             size_t region_size)

#define _decl_f_hvx_scatter(w, ct)                                             \
  void hvx_scatter_f##w(ct *dst, int index, ct src, size_t region_size)

#define _decl_f_hvx_scatter_16(w, ct)                                          \
  void hvx_scatter_f##w##_16(ct *dst, int16_t index, ct src, size_t region_size)

#define _decl_f_hvx_scatter_bf16                                               \
  void hvx_scatter_bf16(__bf16 *dst, int index, __bf16 src, size_t region_size)

#define _decl_f_hvx_scatter_bf16_16                                            \
  void hvx_scatter_bf16_16(__bf16 *dst, int16_t index, __bf16 src,             \
                           size_t region_size)

_decl_hvx_scatter(8);
_decl_hvx_scatter(16);
_decl_hvx_scatter(32);
_decl_hvx_scatter(64);
_decl_hvx_scatter_16(8);
_decl_hvx_scatter_16(16);
#if __has_bf16__
_decl_f_hvx_scatter_bf16;
_decl_f_hvx_scatter_bf16_16;
#endif
#if __has_Float16__
_decl_f_hvx_scatter(16, _Float16);
_decl_f_hvx_scatter_16(16, _Float16);
#endif
_decl_f_hvx_scatter(32, float);
_decl_f_hvx_scatter(64, double);

#undef _decl_hvx_scatter
#undef _decl_hvx_scatter_16
#undef _decl_f_hvx_scatter
#undef _decl_f_hvx_scatter_16
#undef _decl_f_hvx_scatter_bf16
#undef _decl_f_hvx_scatter_bf16_16

#ifdef __cplusplus
} // extern "C"
#endif

// ripple_vector_candidate definitions.

#include <ripple/zip.h>

namespace ripple {
namespace detail {
template <typename T> struct make_unsigned;
template <> struct make_unsigned<int8_t> {
  using type = uint8_t;
};
template <> struct make_unsigned<int16_t> {
  using type = uint16_t;
};
template <> struct make_unsigned<int32_t> {
  using type = uint32_t;
};
template <> struct make_unsigned<int64_t> {
  using type = uint64_t;
};
} // namespace detail
} // namespace ripple

#define HVX_GATHER_32(dst, src, index, region_last_byte)                       \
  Q6_vgather_ARMVw((dst), (uint32_t)(src), (region_last_byte), (index))

#define HVX_GATHER_MASKED_32(dst, src, index, region_last_byte, mask)          \
  Q6_vgather_AQRMVw((dst), Q6_Q_vand_VR((mask), ~0), (uint32_t)(src),          \
                    (region_last_byte), (index))

#define HVX_GATHER_16(dst, src, index, region_last_byte)                       \
  Q6_vgather_ARMVh((dst), (uint32_t)(src), (region_last_byte), (index))

#define HVX_GATHER_MASKED_16(dst, src, index, region_last_byte, mask)          \
  Q6_vgather_AQRMVh((dst), Q6_Q_vand_VR((mask), ~0), (uint32_t)(src),          \
                    (region_last_byte), (index))

#define HVX_GATHER_16_32(dst, src, index, region_last_byte)                    \
  Q6_vgather_ARMWw((dst), (uint32_t)(src), (region_last_byte), (index))

#define HVX_GATHER_MASKED_16_32(dst, src, index, region_last_byte, mask)       \
  Q6_vgather_AQRMWw((dst), Q6_Q_vand_VR((mask), ~0), (uint32_t)(src),          \
                    (region_last_byte), (index))

#define HVX_SCATTER_32(dst, index, src, region_last_byte)                      \
  Q6_vscatter_RMVwV((uint32_t)(dst), (region_last_byte), (index), (src))

#define HVX_SCATTER_MASKED_32(dst, index, src, region_last_byte, mask)         \
  Q6_vscatter_QRMVwV(Q6_Q_vand_VR((mask), ~0), (uint32_t)(dst),                \
                     (region_last_byte), (index), (src))

#define HVX_SCATTER_16(dst, index, src, region_last_byte)                      \
  Q6_vscatter_RMVhV((uint32_t)(dst), (region_last_byte), (index), (src))

#define HVX_SCATTER_MASKED_16(dst, index, src, region_last_byte, mask)         \
  Q6_vscatter_QRMVhV(Q6_Q_vand_VR(mask, ~0), (uint32_t)(dst),                  \
                     (region_last_byte), (index), (src))

#define HVX_SCATTER_16_32(dst, index, src, region_last_byte)                   \
  Q6_vscatter_RMWwV((uint32_t)(dst), (region_last_byte), (index), (src))

#define HVX_SCATTER_MASKED_16_32(dst, index, src, region_last_byte, mask)      \
  Q6_vscatter_QRMWwV(Q6_Q_vand_VR(mask, ~0), (uint32_t)(dst),                  \
                     (region_last_byte), (index), (src))

#define SCATTER_RELEASE(ADDR)                                                  \
  asm volatile("vmem(%0 + #0):scatter_release\n" ::"r"(ADDR));

#if SYNC_VECTOR
#define HVX_SCATTER_SYNC(dst)                                                  \
  {                                                                            \
    SCATTER_RELEASE(dst);                                                      \
    volatile HVX_Vector vDummy = *(HVX_Vector *)dst;                           \
  }
#else
#define HVX_SCATTER_SYNC(dst)                                                  \
  {                                                                            \
    SCATTER_RELEASE(dst);                                                      \
  }
#endif

#if SYNC_VECTOR
#define HVX_GATHER_SYNC(dst)                                                   \
  {                                                                            \
    volatile HVX_Vector vDummy = *(HVX_Vector *)(dst);                         \
  }
#else
#define HVX_GATHER_SYNC(dst)                                                   \
  { /* No-op when SYNC_VECTOR is not defined */                                \
  }
#endif

inline HVX_Vector hvx_pack_mask(HVX_VectorPair mask) {
  return Q6_Vh_vpack_VwVw_sat(Q6_V_hi_W(mask), Q6_V_lo_W(mask));
}

inline v64i32 Q6_vasl(v64i32 x, unsigned amt) {
  return Q6_W_vcombine_VV(Q6_Vw_vasl_VwR(Q6_V_hi_W(x), amt),
                          Q6_Vw_vasl_VwR(Q6_V_lo_W(x), amt));
}

inline v64i16 Q6_vlsr(v64i16 x, unsigned amt) {
  return Q6_Vuh_vlsr_VuhR(x, amt);
}
inline v32i32 Q6_vlsr(v32i32 x, unsigned amt) {
  return Q6_Vuw_vlsr_VuwR(x, amt);
}
inline v64i16 Q6_vasl(v64i16 x, unsigned amt) { return Q6_Vh_vasl_VhR(x, amt); }

__attribute__((always_inline)) inline void
Q6_gather(void *dst, const void *src, size_t last_byte, v64i16 index) {
  HVX_GATHER_16(dst, src, index, last_byte);
}
__attribute__((always_inline)) inline void Q6_gather(void *dst, const void *src,
                                                     size_t last_byte,
                                                     v64i16 index,
                                                     v64i16 mask) {
  HVX_GATHER_MASKED_16(dst, src, index, last_byte, mask);
}
__attribute__((always_inline)) inline void
Q6_gather(void *dst, const void *src, size_t last_byte, v64i32 index) {
  index = Q6_W_vdeal_VVR(Q6_V_hi_W(index), Q6_V_lo_W(index), 128 - 4);
  HVX_GATHER_16_32(dst, src, index, last_byte);
}
__attribute__((always_inline)) inline void Q6_gather(void *dst, const void *src,
                                                     size_t last_byte,
                                                     v64i32 index,
                                                     v64i16 mask) {
  index = Q6_W_vdeal_VVR(Q6_V_hi_W(index), Q6_V_lo_W(index), 128 - 4);
  HVX_GATHER_MASKED_16_32(dst, src, index, last_byte, mask);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v64i16 offsets) {
  HVX_SCATTER_16(dst, offsets, src, last_byte);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v64i16 offsets,
           v64i16 mask) {
  HVX_SCATTER_MASKED_16(dst, offsets, src, last_byte, mask);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v64i32 offsets) {
  v64i16 adjusted_src = Q6_Vh_vshuff_Vh(src);
  HVX_SCATTER_16_32(dst, offsets, adjusted_src, last_byte);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v64i32 offsets,
           v64i16 mask) {
  v64i32 adjusted_offsets =
      Q6_W_vdeal_VVR(Q6_V_hi_W(offsets), Q6_V_lo_W(offsets), -4);
  HVX_SCATTER_MASKED_16_32(dst, adjusted_offsets, src, last_byte, mask);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v32i32 offsets) {
  HVX_SCATTER_32(dst, offsets, src, last_byte);
}
__attribute__((always_inline)) inline void
Q6_scatter(void *dst, size_t last_byte, HVX_Vector src, v32i32 index,
           v32i32 mask) {
  HVX_SCATTER_MASKED_32(dst, index, src, last_byte, mask);
}

using namespace rzip;

static size_t pairs_from_lo(size_t dst_idx,
                            size_t block_size RIPPLE_MAYBE_UNUSED) {
  return dst_idx / 2;
}
static size_t pairs_from_hi(size_t dst_idx, size_t block_size) {
  size_t h = block_size / 2;
  return h + dst_idx / 2;
}

// _____________________________ vgather ______________________________________

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i64(int64_t *dst,
                                                   const int64_t *src,
                                                   v32i32 index,
                                                   size_t region_size) {
  auto BS = ripple_set_block_shape(0, 32);
  size_t v = ripple_id(BS, 0);
  auto idx = vec_to_ripple<32, int32_t>(BS, index);
  int32_t idx_lo = ripple_shuffle(idx, pairs_from_lo) * 2;
  int32_t idx_hi = ripple_shuffle(idx, pairs_from_hi) * 2;
  idx_lo |= v & 1;
  idx_hi |= v & 1;
  auto index_lo = ripple_to_vec<32, int32_t>(BS, idx_lo * sizeof(uint32_t));
  auto index_hi = ripple_to_vec<32, int32_t>(BS, idx_hi * sizeof(uint32_t));
  int32_t region_last_byte = (region_size << 3) - 1;
  HVX_GATHER_32(dst, src, index_lo, region_last_byte);
  HVX_GATHER_32(dst + 16, src, index_hi, region_last_byte);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i64(int64_t *dst, const int64_t *src, v32i32 index,
                           size_t region_size, v32i32 mask) {
  auto BS = ripple_set_block_shape(0, 32);
  size_t v = ripple_id(BS, 0);
  auto idx = vec_to_ripple<32, int32_t>(BS, index);
  auto msk = vec_to_ripple<32, int32_t>(BS, mask);
  int32_t idx_lo = ripple_shuffle(idx, pairs_from_lo) * 2;
  int32_t idx_hi = ripple_shuffle(idx, pairs_from_hi) * 2;
  int32_t msk_lo = ripple_shuffle(msk, pairs_from_lo);
  int32_t msk_hi = ripple_shuffle(msk, pairs_from_hi);
  idx_lo |= v & 1;
  idx_hi |= v & 1;
  int32_t region_last_byte = (region_size << 3) - 1;
  auto index_lo = ripple_to_vec<32, int32_t>(BS, idx_lo * sizeof(uint32_t));
  auto mask_lo = ripple_to_vec<32, int32_t>(BS, msk_lo ? -1 : 0);
  HVX_GATHER_MASKED_32(dst, src, index_lo, region_last_byte, mask_lo);
  auto index_hi = ripple_to_vec<32, int32_t>(BS, idx_hi * sizeof(uint32_t));
  auto mask_hi = ripple_to_vec<32, int32_t>(BS, msk_hi ? -1 : 0);
  HVX_GATHER_MASKED_32(dst + 16, src, index_hi, region_last_byte, mask_hi);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i32(int32_t *dst,
                                                   const int32_t *src,
                                                   v32i32 index,
                                                   size_t region_size) {
  int32_t region_last_byte = (region_size << 2) - 1;
  HVX_GATHER_32(dst, src, Q6_Vw_vasl_VwR(index, 2), region_last_byte);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i32(int32_t *dst, const int32_t *src, v32i32 index,
                           size_t region_size, v32i32 mask) {
  int32_t region_last_byte = (region_size << 2) - 1;
  HVX_GATHER_MASKED_32(dst, src, Q6_Vw_vasl_VwR(index, 2), region_last_byte,
                       Q6_Q_vcmp_gt_VuwVuw(mask, Q6_V_vsplat_R(0)));
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i16(int16_t *dst,
                                                   const int16_t *src,
                                                   v64i32 index,
                                                   size_t region_size) {
  int32_t region_last_byte = (region_size << 1) - 1;
  Q6_gather(dst, src, region_last_byte, Q6_vasl(index, 1));
  HVX_GATHER_SYNC(dst);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i16(int16_t *dst, const int16_t *src, v64i32 index,
                           size_t region_size, v64i32 mask) {
  int32_t region_last_byte = (region_size << 1) - 1;
  Q6_gather(dst, src, region_last_byte, Q6_vasl(index, 1),
            Q6_Q_vcmp_gt_VuhVuh(hvx_pack_mask(mask), Q6_Vh_vsplat_R(0)));
  HVX_GATHER_SYNC(dst);
}

// sub-element gather helpers
template <unsigned Slice, size_t N, typename T, typename ElementT,
          typename IndexT>
[[gnu::always_inline]] inline void sub_gather(ripple_block_t BS, T *dst,
                                              const T *src, size_t last_byte,
                                              IndexT index, int mask) {
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto sliced_mask = ripple_slice(int32_t(mask), -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  if (sliced_mask == 0)
    bitmask = 0;
  Q6_gather(dst, src, last_byte,
            ripple_to_vec<N, IndexT>(BS, sliced_index - Slice),
            ripple_to_vec<N, ElementT>(BS, bitmask));
}

template <size_t N, typename T, typename ElementT, typename IndexT>
[[gnu::always_inline]] inline void
sub_gather_zero(ripple_block_t BS, T *dst, const T *src,
                size_t last_byte RIPPLE_MAYBE_UNUSED, IndexT index, int mask) {
  constexpr unsigned Slice = 1;
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto sliced_mask = ripple_slice(int32_t(mask), -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  ElementT zero_element = src[0] << (sizeof(T) * 8 * Slice);
  if (sliced_index != 0 || sliced_mask == 0)
    bitmask = 0;
  auto hvx_mask = ripple_to_vec<N, ElementT>(BS, bitmask);
  auto hvx_value = ripple_to_vec<N, ElementT>(BS, zero_element);
  Q6_vmem_QRIV(hvx_mask, dst, hvx_value);
}

template <unsigned Slice, size_t N, typename T, typename ElementT,
          typename IndexT>
[[gnu::always_inline]] inline void sub_gather(ripple_block_t BS, T *dst,
                                              const T *src, size_t last_byte,
                                              IndexT index) {
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  Q6_gather(dst, src, last_byte,
            ripple_to_vec<N, IndexT>(BS, sliced_index - Slice),
            ripple_to_vec<N, ElementT>(BS, bitmask));
}

template <size_t N, typename T, typename ElementT, typename IndexT>
[[gnu::always_inline]] inline void
sub_gather_zero(ripple_block_t BS, T *dst, const T *src,
                size_t last_byte RIPPLE_MAYBE_UNUSED, IndexT index) {
  constexpr unsigned Slice = 1;
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  ElementT zero_element = src[0] << (sizeof(T) * 8 * Slice);
  if (sliced_index != 0)
    bitmask = 0;
  auto hvx_mask = ripple_to_vec<N, ElementT>(BS, bitmask);
  auto hvx_value = ripple_to_vec<N, ElementT>(BS, zero_element);
  Q6_vmem_QRIV(hvx_mask, dst, hvx_value);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i8(int8_t *dst,
                                                  const int8_t *src,
                                                  v128i32 index,
                                                  size_t region_size) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int32_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  sub_gather<0, 64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx);
  sub_gather<1, 64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx);
  sub_gather_zero<64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i8(int8_t *dst, const int8_t *src, v128i32 index,
                          size_t region_size, v128i32 mask) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int32_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  auto msk = vec_to_ripple_2d<128, int32_t>(BS, mask);
  msk = ripple_shuffle(msk, shuffle_unzip<2, 0, 0>);
  sub_gather<0, 64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx,
                                              msk);
  sub_gather<1, 64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx,
                                              msk);
  sub_gather_zero<64, int8_t, int16_t, int32_t>(BS, dst, src, last_byte, idx,
                                                msk);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i16_16(int16_t *dst,
                                                      const int16_t *src,
                                                      v64i16 index,
                                                      size_t region_size) {
  int32_t region_last_byte = (region_size << 1) - 1;
  Q6_gather(dst, src, region_last_byte, Q6_vasl(index, 1));
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i16_16(int16_t *dst, const int16_t *src, v64i16 index,
                              size_t region_size, v64i16 mask) {
  int32_t region_last_byte = (region_size << 1) - 1;
  HVX_GATHER_MASKED_16(dst, src, Q6_vasl(index, 1), region_last_byte,
                       Q6_Q_vcmp_gt_VuhVuh(mask, Q6_Vh_vsplat_R(0)));
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_i8_16(int8_t *dst,
                                                     const int8_t *src,
                                                     v128i16 index,
                                                     size_t region_size) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int16_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  sub_gather<0, 64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx);
  sub_gather<1, 64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx);
  sub_gather_zero<64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_gather_i8_16(int8_t *dst, const int8_t *src, v128i16 index,
                             size_t region_size, v128i16 mask) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int16_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  auto msk = vec_to_ripple_2d<128, int16_t>(BS, mask);
  msk = ripple_shuffle(msk, shuffle_unzip<2, 0, 0>);
  sub_gather<0, 64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx,
                                              msk);
  sub_gather<1, 64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx,
                                              msk);
  sub_gather_zero<64, int8_t, int16_t, int16_t>(BS, dst, src, last_byte, idx,
                                                msk);
}

// _____________________________ vscatter _____________________________________

template <unsigned Slice, size_t N, typename ElementT, typename IndexT,
          typename T>
[[gnu::always_inline]] inline void partial_scatter(ripple_block_t BS, T *dst,
                                                   size_t last_byte,
                                                   ElementT src, IndexT idx) {
  ElementT src_slice = ripple_slice(src, -1, Slice);
  int offset = idx * sizeof(T) + Slice * sizeof(ElementT);
  Q6_scatter(dst, last_byte, ripple_to_vec<N, ElementT>(BS, src_slice),
             ripple_to_vec<N, IndexT>(BS, offset));
}

template <unsigned Slice, size_t N, typename ElementT, typename IndexT,
          typename MaskT, typename T>
[[gnu::always_inline]] inline void
partial_scatter(ripple_block_t BS, T *dst, size_t last_byte, ElementT src,
                IndexT idx, MaskT msk) {
  ElementT src_slice = ripple_slice(src, -1, Slice);
  int offset = idx * sizeof(T) + Slice * sizeof(ElementT);
  Q6_scatter(dst, last_byte, ripple_to_vec<N, ElementT>(BS, src_slice),
             ripple_to_vec<N, IndexT>(BS, offset),
             ripple_to_vec<N, MaskT>(BS, msk != 0 ? -1 : 0));
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i64(int64_t *dst, v32i32 index,
                                                    v32i64 src,
                                                    size_t region_size) {
  size_t last_byte = region_size * sizeof(int64_t) - 1;
  auto BS = ripple_set_block_shape(0, 32, 2);
  auto s = vec_to_ripple_2d<64, int32_t>(BS, src);
  s = ripple_shuffle(s, shuffle_unzip<2, 0, 0>);
  auto BS2 = ripple_set_block_shape(0, 32);
  auto idx = vec_to_ripple<32, int32_t>(BS2, index);
  partial_scatter<0, 32, int32_t, int32_t>(BS2, dst, last_byte, s, idx);
  partial_scatter<1, 32, int32_t, int32_t>(BS2, dst, last_byte, s, idx);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i64(int64_t *dst, v32i32 index, v32i64 src,
                            size_t region_size, v32i32 mask) {
  size_t last_byte = region_size * sizeof(int64_t) - 1;
  auto BS = ripple_set_block_shape(0, 32, 2);
  auto s = vec_to_ripple_2d<64, int32_t>(BS, src);
  s = ripple_shuffle(s, shuffle_unzip<2, 0, 0>);
  auto BS2 = ripple_set_block_shape(0, 32);
  auto idx = vec_to_ripple<32, int32_t>(BS2, index);
  auto msk = vec_to_ripple<32, int32_t>(BS2, mask);
  partial_scatter<0, 32, int32_t, int32_t, int32_t>(BS2, dst, last_byte, s, idx,
                                                    msk);
  partial_scatter<1, 32, int32_t, int32_t, int32_t>(BS2, dst, last_byte, s, idx,
                                                    msk);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i32(int32_t *dst, v32i32 index,
                                                    v32i32 src,
                                                    size_t region_size) {
  int32_t region_last_byte = (region_size << 2) - 1;
  HVX_SCATTER_32(dst, Q6_Vw_vasl_VwR(index, 2), src, region_last_byte);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i32(int32_t *dst, v32i32 index, v32i32 src,
                            size_t region_size, v32i32 mask) {
  int32_t region_last_byte = (region_size << 2) - 1;
  HVX_SCATTER_MASKED_32(dst, Q6_Vw_vasl_VwR(index, 2), src, region_last_byte,
                        Q6_Q_vcmp_gt_VuwVuw(mask, Q6_V_vsplat_R(0)));
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i16(int16_t *dst, v64i32 index,
                                                    v64i16 src,
                                                    size_t region_size) {
  size_t region_last_byte = (region_size << 1) - 1;
  v64i32 offsets = Q6_vasl(index, 1);
  Q6_scatter(dst, region_last_byte, src, offsets);
  HVX_SCATTER_SYNC(dst);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i16(int16_t *dst, v64i32 index, v64i16 src,
                            size_t region_size, v64i32 mask) {
  int32_t region_last_byte = (region_size << 1) - 1;
  v64i32 offsets = Q6_vasl(index, 1);
  Q6_scatter(dst, region_last_byte, src, offsets,
             Q6_Q_vcmp_gt_VuhVuh(hvx_pack_mask(mask), Q6_Vh_vsplat_R(0)));
  HVX_SCATTER_SYNC(dst);
}

// sub-element scatter helpers
template <unsigned Slice, size_t N, typename ElementT, typename IndexT,
          typename T>
[[gnu::always_inline]] inline void sub_scatter(ripple_block_t BS, T *dst,
                                               size_t last_byte, HVX_Vector src,
                                               IndexT index) {
  auto sliced_index = ripple_slice(index, -1, Slice);
  IndexT offset = (sliced_index - Slice) * sizeof(T);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  Q6_scatter(dst, last_byte, src, ripple_to_vec<N, IndexT>(BS, offset),
             ripple_to_vec<N, ElementT>(BS, bitmask));
}

template <size_t N, typename ElementT, typename IndexT, typename T>
[[gnu::always_inline]] inline void
sub_scatter_zero(ripple_block_t BS, T *dst, size_t last_byte, HVX_Vector src,
                 IndexT index) {
  constexpr unsigned Slice = 1;
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  if (sliced_index != 0)
    bitmask = 0;
  v64i16 zero_offsets = Q6_V_vzero();
  // The mask must be spelled v64i16 explicitly: an unqualified HVX_Vector
  // converts to both the v64i16 and v32i32 mask overloads at equal rank.
  v64i16 shifted_mask = Q6_Vuh_vlsr_VuhR(
      (ripple_to_vec<N, ElementT>(BS, bitmask)), sizeof(T) * 8);
  Q6_scatter(dst, last_byte, Q6_Vuh_vlsr_VuhR(src, sizeof(T) * 8), zero_offsets,
             shifted_mask);
}

template <unsigned Slice, size_t N, typename ElementT, typename IndexT,
          typename T>
[[gnu::always_inline]] inline void sub_scatter(ripple_block_t BS, T *dst,
                                               size_t last_byte, HVX_Vector src,
                                               IndexT index, int mask) {
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto sliced_mask = ripple_slice(int32_t(mask), -1, Slice);
  IndexT offset = (sliced_index - Slice) * sizeof(T);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  if (sliced_mask == 0)
    bitmask = 0;
  Q6_scatter(dst, last_byte, src, ripple_to_vec<N, IndexT>(BS, offset),
             ripple_to_vec<N, ElementT>(BS, bitmask));
}

template <size_t N, typename ElementT, typename IndexT, typename T>
[[gnu::always_inline]] inline void
sub_scatter_zero(ripple_block_t BS, T *dst, size_t last_byte, HVX_Vector src,
                 IndexT index, int mask) {
  constexpr unsigned Slice = 1;
  auto sliced_index = ripple_slice(index, -1, Slice);
  auto sliced_mask = ripple_slice(int32_t(mask), -1, Slice);
  auto bitmask = typename ripple::detail::make_unsigned<T>::type(-1U)
                 << (sizeof(T) * 8 * Slice);
  if (sliced_index != 0 || sliced_mask == 0)
    bitmask = 0;
  v64i16 zero_offsets = Q6_V_vzero();
  // The mask must be spelled v64i16 explicitly: an unqualified HVX_Vector
  // converts to both the v64i16 and v32i32 mask overloads at equal rank.
  v64i16 shifted_mask = Q6_Vuh_vlsr_VuhR(
      (ripple_to_vec<N, ElementT>(BS, bitmask)), sizeof(T) * 8);
  Q6_scatter(dst, last_byte, Q6_Vuh_vlsr_VuhR(src, sizeof(T) * 8), zero_offsets,
             shifted_mask);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i8(int8_t *dst, v128i32 index,
                                                   v128i8 src,
                                                   size_t region_size) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int32_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  sub_scatter<0, 64, int16_t, int32_t>(BS, dst, last_byte, src, idx);
  sub_scatter<1, 64, int16_t, int32_t>(BS, dst, last_byte, src, idx);
  sub_scatter_zero<64, int16_t, int32_t>(BS, dst, last_byte, src, idx);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i8(int8_t *dst, v128i32 index, v128i8 src,
                           size_t region_size, v128i32 mask) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int32_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  auto msk = vec_to_ripple_2d<128, int32_t>(BS, mask);
  msk = ripple_shuffle(msk, shuffle_unzip<2, 0, 0>);
  sub_scatter<0, 64, int16_t, int32_t>(BS, dst, last_byte, src, idx, msk);
  sub_scatter<1, 64, int16_t, int32_t>(BS, dst, last_byte, src, idx, msk);
  sub_scatter_zero<64, int16_t, int32_t>(BS, dst, last_byte, src, idx, msk);
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i16_16(int16_t *dst,
                                                       v64i16 index, v64i16 src,
                                                       size_t region_size) {
  int32_t region_last_byte = (region_size << 1) - 1;
  Q6_scatter(dst, region_last_byte, src, Q6_vasl(index, 1));
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i16_16(int16_t *dst, v64i16 index, v64i16 src,
                               size_t region_size, v64i16 mask) {
  int32_t region_last_byte = (region_size << 1) - 1;
  HVX_SCATTER_MASKED_16(dst, Q6_vasl(index, 1), src, region_last_byte,
                        Q6_Q_vcmp_gt_VuhVuh(mask, Q6_V_vsplat_R(0)));
}

RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_i8_16(int8_t *dst,
                                                      v128i16 index, v128i8 src,
                                                      size_t region_size) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int16_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  sub_scatter<0, 64, int16_t, int16_t>(BS, dst, last_byte, src, idx);
  sub_scatter<1, 64, int16_t, int16_t>(BS, dst, last_byte, src, idx);
  sub_scatter_zero<64, int16_t, int16_t>(BS, dst, last_byte, src, idx);
}

RIPPLE_CANDIDATE_INLINE void
ripple_mask_hvx_scatter_i8_16(int8_t *dst, v128i16 index, v128i8 src,
                              size_t region_size, v128i16 mask) {
  auto BS = ripple_set_block_shape(0, 64, 2);
  size_t last_byte = region_size - 1;
  auto idx = vec_to_ripple_2d<128, int16_t>(BS, index);
  idx = ripple_shuffle(idx, shuffle_unzip<2, 0, 0>);
  auto msk = vec_to_ripple_2d<128, int16_t>(BS, mask);
  msk = ripple_shuffle(msk, shuffle_unzip<2, 0, 0>);
  sub_scatter<0, 64, int16_t, int16_t>(BS, dst, last_byte, src, idx, msk);
  sub_scatter<1, 64, int16_t, int16_t>(BS, dst, last_byte, src, idx, msk);
  sub_scatter_zero<64, int16_t, int16_t>(BS, dst, last_byte, src, idx, msk);
}

// Float and Unsigned gather/scatter variants (wrappers that reuse int
// implementations)

#define _decl_float_gather(CT, T, W, N)                                        \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_##T##W(                       \
      CT *dst, const CT *src, v##N##i32 index, size_t region_size) {           \
    ripple_hvx_gather_i##W((int##W##_t *)dst, (int##W##_t *)src, index,        \
                           region_size);                                       \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_gather_##T##W(                  \
      CT *dst, const CT *src, v##N##i32 index, size_t region_size,             \
      v##N##i32 mask) {                                                        \
    ripple_mask_hvx_gather_i##W((int##W##_t *)dst, (int##W##_t *)src, index,   \
                                region_size, mask);                            \
  }

#define _decl_float_gather_16(CT, T, W, N)                                     \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_##T##W##_16(                  \
      CT *dst, const CT *src, v##N##i16 index, size_t region_size) {           \
    ripple_hvx_gather_i##W##_16((int##W##_t *)dst, (int##W##_t *)src, index,   \
                                region_size);                                  \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_gather_##T##W##_16(             \
      CT *dst, const CT *src, v##N##i16 index, size_t region_size,             \
      v##N##i16 mask) {                                                        \
    ripple_mask_hvx_gather_i##W##_16((int##W##_t *)dst, (int##W##_t *)src,     \
                                     index, region_size, mask);                \
  }

#define _decl_unsigned_gather(W, N)                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_u##W(                         \
      int##W##_t *dst, const int##W##_t *src, v##N##i32 index,                 \
      size_t region_size) {                                                    \
    ripple_hvx_gather_i##W(dst, src, index, region_size);                      \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_gather_u##W(                    \
      int##W##_t *dst, const int##W##_t *src, v##N##i32 index,                 \
      size_t region_size, v##N##i32 mask) {                                    \
    ripple_mask_hvx_gather_i##W(dst, src, index, region_size, mask);           \
  }

#define _decl_unsigned_gather_16(W, N)                                         \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_gather_u##W##_16(                    \
      int##W##_t *dst, const int##W##_t *src, v##N##i16 index,                 \
      size_t region_size) {                                                    \
    ripple_hvx_gather_i##W##_16(dst, src, index, region_size);                 \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_gather_u##W##_16(               \
      int##W##_t *dst, const int##W##_t *src, v##N##i16 index,                 \
      size_t region_size, v##N##i16 mask) {                                    \
    ripple_mask_hvx_gather_i##W##_16(dst, src, index, region_size, mask);      \
  }

// clang-format off
_decl_float_gather(double, f, 64, 32)
_decl_float_gather(float, f, 32, 32)
_decl_float_gather(_Float16, f, 16, 64)
_decl_float_gather_16(_Float16, f, 16, 64)
#ifndef __has_bf16__
#define __ripple_sg_defined_has_bf16 1
#define __has_bf16__ 0
#endif
#if __has_bf16__
_decl_float_gather(__bf16, bf, 16, 64)
_decl_float_gather_16(__bf16, bf, 16, 64)
#endif

_decl_unsigned_gather(64, 32)
_decl_unsigned_gather(32, 32)
_decl_unsigned_gather(16, 64)
_decl_unsigned_gather(8, 128)
_decl_unsigned_gather_16(16, 64)
_decl_unsigned_gather_16(8, 128)

#undef _decl_float_gather
#undef _decl_float_gather_16
#undef _decl_unsigned_gather
#undef _decl_unsigned_gather_16
// clang-format on

#define _decl_float_scatter(CT, T, W, N)                                       \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_##T##W(                      \
      CT *dst, v##N##i32 index, v##N##T##W src, size_t region_size) {          \
    v##N##i##W int_src;                                                        \
    __builtin_memcpy(&int_src, &src, sizeof(src));                             \
    ripple_hvx_scatter_i##W((int##W##_t *)dst, index, int_src, region_size);   \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_scatter_##T##W(                 \
      CT *dst, v##N##i32 index, v##N##T##W src, size_t region_size,            \
      v##N##i32 mask) {                                                        \
    v##N##i##W int_src;                                                        \
    __builtin_memcpy(&int_src, &src, sizeof(src));                             \
    ripple_mask_hvx_scatter_i##W((int##W##_t *)dst, index, int_src,            \
                                 region_size, mask);                           \
  }

#define _decl_float_scatter_16(CT, T, W, N)                                    \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_##T##W##_16(                 \
      CT *dst, v##N##i16 index, v##N##T##W src, size_t region_size) {          \
    v##N##i##W int_src;                                                        \
    __builtin_memcpy(&int_src, &src, sizeof(src));                             \
    ripple_hvx_scatter_i##W##_16((int##W##_t *)dst, index, int_src,            \
                                 region_size);                                 \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_scatter_##T##W##_16(            \
      CT *dst, v##N##i16 index, v##N##T##W src, size_t region_size,            \
      v##N##i16 mask) {                                                        \
    v##N##i##W int_src;                                                        \
    __builtin_memcpy(&int_src, &src, sizeof(src));                             \
    ripple_mask_hvx_scatter_i##W##_16((int##W##_t *)dst, index, int_src,       \
                                      region_size, mask);                      \
  }

#define _decl_unsigned_scatter(W, N)                                           \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_u##W(                        \
      int##W##_t *dst, v##N##i32 index, v##N##i##W src, size_t region_size) {  \
    ripple_hvx_scatter_i##W(dst, index, src, region_size);                     \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_scatter_u##W(                   \
      int##W##_t *dst, v##N##i32 index, v##N##i##W src, size_t region_size,    \
      v##N##i32 mask) {                                                        \
    ripple_mask_hvx_scatter_i##W(dst, index, src, region_size, mask);          \
  }

#define _decl_unsigned_scatter_16(W, N)                                        \
  RIPPLE_CANDIDATE_INLINE void ripple_hvx_scatter_u##W##_16(                   \
      int##W##_t *dst, v##N##i16 index, v##N##i##W src, size_t region_size) {  \
    ripple_hvx_scatter_i##W##_16(dst, index, src, region_size);                \
  }                                                                            \
  RIPPLE_CANDIDATE_INLINE void ripple_mask_hvx_scatter_u##W##_16(              \
      int##W##_t *dst, v##N##i16 index, v##N##i##W src, size_t region_size,    \
      v##N##i16 mask) {                                                        \
    ripple_mask_hvx_scatter_i##W##_16(dst, index, src, region_size, mask);     \
  }

    // clang-format off
_decl_float_scatter(double, f, 64, 32)
_decl_float_scatter(float, f, 32, 32)
_decl_float_scatter(_Float16, f, 16, 64)
_decl_float_scatter_16(_Float16, f, 16, 64)
#if __has_bf16__
_decl_float_scatter(__bf16, bf, 16, 64)
_decl_float_scatter_16(__bf16, bf, 16, 64)
#endif

_decl_unsigned_scatter(64, 32)
_decl_unsigned_scatter(32, 32)
_decl_unsigned_scatter(16, 64)
_decl_unsigned_scatter(8, 128)
_decl_unsigned_scatter_16(16, 64)
_decl_unsigned_scatter_16(8, 128)

#undef _decl_float_scatter
#undef _decl_float_scatter_16
#undef _decl_unsigned_scatter
#undef _decl_unsigned_scatter_16
// clang-format on

#undef HVX_GATHER_32
#undef HVX_GATHER_MASKED_32
#undef HVX_GATHER_16
#undef HVX_GATHER_MASKED_16
#undef HVX_GATHER_16_32
#undef HVX_GATHER_MASKED_16_32
#undef HVX_SCATTER_32
#undef HVX_SCATTER_MASKED_32
#undef HVX_SCATTER_16
#undef HVX_SCATTER_MASKED_16
#undef HVX_SCATTER_16_32
#undef HVX_SCATTER_MASKED_16_32
#undef SCATTER_RELEASE
#undef HVX_SCATTER_SYNC
#undef HVX_GATHER_SYNC

#ifndef __cplusplus
#if __has_bf16__
#define __extra_bf16_scatter(dst, index, src, region_size)                     \
  , __bf16 : hvx_scatter_bf16((dst), (index), (src), (region_size))
#else
#define __extra_bf16_scatter(dst, index, src, region_size)
#endif

#if __has_Float16__
#define __extra_f16_scatter(dst, index, src, region_size)                      \
  , _Float16 : hvx_scatter_f16((dst), (index), (src), (region_size))
#else
#define __extra_f16_scatter(dst, index, src, region_size)
#endif

#define hvx_scatter(dst, index, src, region_size)                              \
  _Generic((src),                                                              \
      int8_t: _Generic((index),                                                \
          int16_t: hvx_scatter_i8_16((dst), (index), (src), (region_size)),    \
          int32_t: hvx_scatter_i8((dst), (index), (src), (region_size))),      \
      int16_t: _Generic((index),                                               \
          int16_t: hvx_scatter_i16_16((dst), (index), (src), (region_size)),   \
          int32_t: hvx_scatter_i16((dst), (index), (src), (region_size))),     \
      int32_t: hvx_scatter_i32((dst), (index), (src), (region_size)),          \
      int64_t: hvx_scatter_i64((dst), (index), (src), (region_size)),          \
      uint8_t: _Generic((index),                                               \
          int16_t: hvx_scatter_u8_16((dst), (index), (src), (region_size)),    \
          int32_t: hvx_scatter_u8((dst), (index), (src), (region_size))),      \
      uint16_t: _Generic((index),                                              \
          int16_t: hvx_scatter_u16_16((dst), (index), (src), (region_size)),   \
          int32_t: hvx_scatter_u16((dst), (index), (src), (region_size))),     \
      uint32_t: hvx_scatter_u32((dst), (index), (src), (region_size)),         \
      uint64_t: hvx_scatter_u64((dst), (index), (src), (region_size)),         \
      float: hvx_scatter_f32((dst), (index), (src), (region_size)),            \
      double: hvx_scatter_f64((dst), (index), (src), (region_size))            \
          __extra_bf16_scatter(dst, index, src, region_size)                   \
              __extra_f16_scatter(dst, index, src, region_size))

#else // __cplusplus
#define _decl_hvx_scatter_spec(ct, t)                                          \
  [[gnu::always_inline, gnu::unused]] static void hvx_scatter(                 \
      ct *dst, int index, ct src, size_t region_size) {                        \
    hvx_scatter_##t(dst, index, src, region_size);                             \
  }

    // clang-format off
_decl_hvx_scatter_spec(int8_t, i8)
_decl_hvx_scatter_spec(uint8_t, u8)
_decl_hvx_scatter_spec(int16_t, i16)
_decl_hvx_scatter_spec(uint16_t, u16)
_decl_hvx_scatter_spec(int32_t, i32)
_decl_hvx_scatter_spec(uint32_t, u32)
_decl_hvx_scatter_spec(int64_t, i64)
_decl_hvx_scatter_spec(uint64_t, u64)

_decl_hvx_scatter_spec(float, f32)
_decl_hvx_scatter_spec(double, f64)
#if __has_bf16__
_decl_hvx_scatter_spec(__bf16, bf16)
#endif
#if __has_Float16__
_decl_hvx_scatter_spec(_Float16, f16)
#endif

#undef _decl_hvx_scatter_spec
// clang-format on

#define _decl_hvx_scatter_16_spec(ct, t)                                       \
  [[gnu::always_inline, gnu::unused]] static void hvx_scatter(                 \
      ct *dst, int16_t index, ct src, size_t region_size) {                    \
    hvx_scatter_##t##_16(dst, index, src, region_size);                        \
  }

    // clang-format off
_decl_hvx_scatter_16_spec(int8_t, i8)
_decl_hvx_scatter_16_spec(uint8_t, u8)

_decl_hvx_scatter_16_spec(int16_t, i16)
_decl_hvx_scatter_16_spec(uint16_t, u16)

#if __has_bf16__
_decl_hvx_scatter_16_spec(__bf16, bf16)
#endif
#if __has_Float16__
_decl_hvx_scatter_16_spec(_Float16, f16)
#endif

#undef _decl_hvx_scatter_16_spec
// clang-format on

#endif // __cplusplus

#ifdef __ripple_sg_defined_has_bf16
#undef __has_bf16__
#undef __ripple_sg_defined_has_bf16
#endif
