//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h> // C99 and above

#ifdef __cplusplus
}
#endif

#define __decl_f16_vec_t(VEC_SIZE, VEC_BYTE_WIDTH)                             \
  typedef _Float16 f16t##VEC_SIZE __attribute__((vector_size(VEC_BYTE_WIDTH)))

__decl_f16_vec_t(64, 128);

#undef __decl_f16_vec_t

#define __decl_f32_vec_t(VEC_SIZE, VEC_BYTE_WIDTH)                             \
  typedef float f32t##VEC_SIZE __attribute__((vector_size(VEC_BYTE_WIDTH)))

__decl_f32_vec_t(32, 128);

#undef __decl_f32_vec_t

#define __decl_int_vec_t(EL_WIDTH, VEC_SIZE, VEC_BYTE_WIDTH)                   \
  typedef int##EL_WIDTH##_t i##EL_WIDTH##t##VEC_SIZE                           \
      __attribute__((vector_size(VEC_BYTE_WIDTH)))

#define __decl_uint_vec_t(EL_WIDTH, VEC_SIZE, VEC_BYTE_WIDTH)                  \
  typedef uint##EL_WIDTH##_t u##EL_WIDTH##t##VEC_SIZE                          \
      __attribute__((vector_size(VEC_BYTE_WIDTH)))

// vector_types_no_stdint.h declares the same i32t32/u32t32/... names off its
// own __ripple_i32/u32/... typedefs. On targets where a libc's <stdint.h>
// int32_t doesn't match clang's __INT32_TYPE__ (e.g. hexagon's, which is
// 'long' rather than 'int'), redeclaring them here with a different
// underlying scalar is a hard error, so skip whichever of the two headers
// runs second.
#ifndef __RIPPLE_VECTOR_INT_TYPES_DEFINED
#define __RIPPLE_VECTOR_INT_TYPES_DEFINED

__decl_int_vec_t(32, 32, 128);
__decl_int_vec_t(16, 64, 128);
__decl_int_vec_t(32, 64, 256);

__decl_uint_vec_t(32, 32, 128);
__decl_uint_vec_t(16, 64, 128);

#endif // __RIPPLE_VECTOR_INT_TYPES_DEFINED

#undef __decl_int_vec_t
#undef __decl_uint_vec_t
