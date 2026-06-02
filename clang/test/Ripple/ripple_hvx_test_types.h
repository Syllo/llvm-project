#pragma once

#include "ripple_test.h"

#define RIPPLE_TEST_DECL_F_VEC_T(CT, T, W, N, BW)                              \
  typedef CT v##N##T##W __attribute__((vector_size(BW)))                       \
  __attribute__((aligned(BW)))

#if __has_bf16__
RIPPLE_TEST_DECL_F_VEC_T(__bf16, bf, 16, 64, 128);
RIPPLE_TEST_DECL_F_VEC_T(__bf16, bf, 16, 128, 256);
RIPPLE_TEST_DECL_F_VEC_T(__bf16, bf, 16, 256, 512);
#endif

#if __has_Float16__
RIPPLE_TEST_DECL_F_VEC_T(_Float16, f, 16, 64, 128);
RIPPLE_TEST_DECL_F_VEC_T(_Float16, f, 16, 128, 256);
RIPPLE_TEST_DECL_F_VEC_T(_Float16, f, 16, 256, 512);
#endif

RIPPLE_TEST_DECL_F_VEC_T(float, f, 32, 32, 128);
RIPPLE_TEST_DECL_F_VEC_T(float, f, 32, 64, 256);
RIPPLE_TEST_DECL_F_VEC_T(float, f, 32, 128, 512);
RIPPLE_TEST_DECL_F_VEC_T(float, f, 32, 256, 1024);
RIPPLE_TEST_DECL_F_VEC_T(double, f, 64, 16, 128);
RIPPLE_TEST_DECL_F_VEC_T(double, f, 64, 32, 256);
RIPPLE_TEST_DECL_F_VEC_T(double, f, 64, 64, 512);
RIPPLE_TEST_DECL_F_VEC_T(double, f, 64, 128, 1024);

#undef RIPPLE_TEST_DECL_F_VEC_T

#define RIPPLE_TEST_DECL_INT_VEC_T(EL_WIDTH, VEC_SIZE, VEC_BYTE_WIDTH)         \
  typedef int##EL_WIDTH##_t v##VEC_SIZE##i##EL_WIDTH                           \
      __attribute__((vector_size(VEC_BYTE_WIDTH)));                            \
  typedef uint##EL_WIDTH##_t v##VEC_SIZE##u##EL_WIDTH                          \
      __attribute__((vector_size(VEC_BYTE_WIDTH)))

RIPPLE_TEST_DECL_INT_VEC_T(8, 128, 128);
RIPPLE_TEST_DECL_INT_VEC_T(16, 64, 128);
RIPPLE_TEST_DECL_INT_VEC_T(32, 32, 128);
RIPPLE_TEST_DECL_INT_VEC_T(64, 16, 128);
RIPPLE_TEST_DECL_INT_VEC_T(8, 256, 256);
RIPPLE_TEST_DECL_INT_VEC_T(16, 128, 256);
RIPPLE_TEST_DECL_INT_VEC_T(32, 64, 256);
RIPPLE_TEST_DECL_INT_VEC_T(64, 32, 256);
RIPPLE_TEST_DECL_INT_VEC_T(8, 512, 512);
RIPPLE_TEST_DECL_INT_VEC_T(16, 256, 512);
RIPPLE_TEST_DECL_INT_VEC_T(32, 128, 512);
RIPPLE_TEST_DECL_INT_VEC_T(64, 64, 512);
RIPPLE_TEST_DECL_INT_VEC_T(8, 1024, 1024);
RIPPLE_TEST_DECL_INT_VEC_T(16, 512, 1024);
RIPPLE_TEST_DECL_INT_VEC_T(32, 256, 1024);
RIPPLE_TEST_DECL_INT_VEC_T(64, 128, 1024);

#undef RIPPLE_TEST_DECL_INT_VEC_T
