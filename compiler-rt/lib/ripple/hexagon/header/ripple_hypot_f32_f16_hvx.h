#ifndef RIPPLE_HYPOT_F32_F16_HVX
#define RIPPLE_HYPOT_F32_F16_HVX

//===-------------------- ripple_hypot_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//      Internal header to access hypot f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include "ripple_qhmath_hvx.h"
#include <hexagon_types.h>

#include "ripple_sqrt_f32_f16_hvx.h"

// safe = handles subnormals (off by default; it roughly triples runtime)
// full = 2 runs of Newton's method instead of 1
// This is based on the sqrt implementations. Same considerations apply here.

template <bool safe = true, bool full = false>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_hypot_f32(HVX_Vector a, HVX_Vector b) {
  HVX_Vector a2_qf = Q6_Vqf32_vmpy_VsfVsf(a, a);
  HVX_Vector b2_qf = Q6_Vqf32_vmpy_VsfVsf(b, b);
  HVX_Vector c2_qf = Q6_Vqf32_vadd_Vqf32Vqf32(a2_qf, b2_qf);
  HVX_Vector c2 = Q6_Vsf_equals_Vqf32(c2_qf);
  return ripple_sqrt_f32<safe, full>(c2);
}

template <bool safe = true, bool full = false>
RIPPLE_INTRIN_INLINE HVX_Vector ripple_hypot_f16(HVX_Vector a, HVX_Vector b) {
  HVX_Vector a2_qf = Q6_Vqf16_vmpy_VhfVhf(a, a);
  HVX_Vector b2_qf = Q6_Vqf16_vmpy_VhfVhf(b, b);
  HVX_Vector c2_qf = Q6_Vqf16_vadd_Vqf16Vqf16(a2_qf, b2_qf);
  HVX_Vector c2 = Q6_Vhf_equals_Vqf16(c2_qf);
  return ripple_sqrt_f16<safe, full>(c2);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_hypot_f16_promote(HVX_Vector a,
                                                         HVX_Vector b) {
  HVX_Vector a2_qf = Q6_Vqf16_vmpy_VhfVhf(a, a);
  HVX_Vector b2_qf = Q6_Vqf16_vmpy_VhfVhf(b, b);
  HVX_Vector c2_qf = Q6_Vqf16_vadd_Vqf16Vqf16(a2_qf, b2_qf);
  HVX_Vector c2 = Q6_Vhf_equals_Vqf16(c2_qf);
  return ripple_sqrt_f16_promote(c2);
}

#endif
