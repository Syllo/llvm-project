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
#include "__ripple_vec.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
[[gnu::always_inline, gnu::unused]] static _Float16
hvx_prepare_lut(_Float16 table) {
  return hvx_prepare_lut_128f16(table);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline, gnu::unused]] static __bf16
hvx_prepare_lut(__bf16 table) {
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
[[gnu::always_inline, gnu::unused]] static __bf16
hvx_vlut(unsigned char indices, __bf16 prepared) {
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
[[gnu::always_inline, gnu::unused]] static _Float16
hvx_prepare_lut_double(_Float16 table) {
  return hvx_prepare_lut_256f16(table);
}
#endif // __has_Float16__
#if __has_bf16__
[[gnu::always_inline, gnu::unused]] static __bf16
hvx_prepare_lut_double(__bf16 table) {
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
