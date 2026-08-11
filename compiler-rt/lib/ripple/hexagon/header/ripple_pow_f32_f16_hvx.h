#ifndef RIPPLE_POW_F32_F16_HVX
#define RIPPLE_POW_F32_F16_HVX

//===---------------------- ripple_pow_f32_f16_hvx.h ------------------------===
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------------===
//       Internal header to access pow f32 and f16 vector implementations.
//===------------------------------------------------------------------------===

#include "__ripple_func_attribs.h"
#include <hexagon_types.h>
#include <stdbool.h>

#include "ripple_exp_f32_f16_hvx.h"
#include "ripple_log_f32_f16_hvx.h"

RIPPLE_INTRIN_INLINE HVX_Vector ripple_pow_f32(HVX_Vector a, HVX_Vector b,
                                               bool safe) {
  // multiply by log 2 to change exponential base
  HVX_Vector exp_qf = Q6_Vqf32_vmpy_VsfVsf(b, Q6_V_vsplat_R(0x3f317218));
  HVX_Vector log_qf =
      Q6_Vqf32_vadd_VsfVsf(ripple_fast_log2_f32(a, safe), Q6_V_vzero());
  HVX_Vector log_res_qf = Q6_Vqf32_vmpy_Vqf32Vqf32(log_qf, exp_qf);
  return ripple_expf32_hvx_schraudolph(Q6_Vsf_equals_Vqf32(log_res_qf), safe);
}

RIPPLE_INTRIN_INLINE HVX_Vector ripple_pow_f16(HVX_Vector a, HVX_Vector b,
                                               bool safe, bool promote) {
  // multiply by log 2 to change exponential base
  HVX_Vector exp_qf = Q6_Vqf16_vmpy_VhfVhf(b, Q6_Vh_vsplat_R(0x398c));
  HVX_Vector log_qf =
      Q6_Vqf16_vadd_VhfVhf(ripple_fast_log2_f16(a, safe), Q6_V_vzero());
  HVX_Vector log_res_qf = Q6_Vqf16_vmpy_Vqf16Vqf16(log_qf, exp_qf);
  HVX_Vector log_res_hf = Q6_Vhf_equals_Vqf16(log_res_qf);
  if (promote)
    return ripple_expf16_hvx_promote(log_res_hf, safe);
  else
    return ripple_expf16_hvx_schraudolph(log_res_hf, safe);
}

#endif
