// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %clang -g -O2 -fenable-ripple -emit-llvm -S -o - -ffast-math %s | FileCheck %s --implicit-check-not="warning:"

#include "../ripple_test.h"
#include "external_library.h"
#include <ripple_math.h>

#define gen_check_unary_mathfn(N, OP, LONGTYPE)                                \
  void check_##OP(const LONGTYPE x[N], LONGTYPE y[N]) {                        \
    ripple_block_t BS = ripple_set_block_shape(0, N);                                               \
    int v0 = ripple_id(BS, 0);                                                  \
    y[v0] = OP(x[v0]);                                                         \
  }

#define gen_check_binary_mathfn(N, OP, LONGTYPE)                               \
  void check_##OP(const LONGTYPE x[N], LONGTYPE y[N], LONGTYPE z[N]) {         \
    ripple_block_t BS = ripple_set_block_shape(0, N);                                               \
    int v0 = ripple_id(BS, 0);                                                  \
    z[v0] = OP(x[v0], y[v0]);                                                  \
  }


// CHECK: @check_sqrtf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_sqrtf
gen_check_unary_mathfn(128, sqrtf, float);
// CHECK: @check_sqrt
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_sqrt
gen_check_unary_mathfn(128, sqrt, double);

// CHECK: @check_asinf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_asinf
gen_check_unary_mathfn(128, asinf, float);
// CHECK: @check_asin
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_asin
gen_check_unary_mathfn(128, asin, double);

// CHECK: @check_acosf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_acosf
gen_check_unary_mathfn(128, acosf, float);
// CHECK: @check_acos
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_acos
gen_check_unary_mathfn(128, acos, double);

// CHECK: @check_atanf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_atanf
gen_check_unary_mathfn(128, atanf, float);
// CHECK: @check_atan
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_atan
gen_check_unary_mathfn(128, atan, double);

// CHECK: @check_atan2f
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_atan2f
gen_check_binary_mathfn(128, atan2f, float);
// CHECK: @check_atan2
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_atan2
gen_check_binary_mathfn(128, atan2, double);

// CHECK: @check_sinf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_sinf
gen_check_unary_mathfn(128, sinf, float);
// CHECK: @check_sin
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_sin
gen_check_unary_mathfn(128, sin, double);

// CHECK: @check_cosf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_cosf
gen_check_unary_mathfn(128, cosf, float);
// CHECK: @check_cos
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_cos
gen_check_unary_mathfn(128, cos, double);

// CHECK: @check_tanf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_tanf
gen_check_unary_mathfn(128, tanf, float);
// CHECK: @check_tan
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_tan
gen_check_unary_mathfn(128, tan, double);

// CHECK: @check_sinhf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_sinhf
gen_check_unary_mathfn(128, sinhf, float);
// CHECK: @check_sinh
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_sinh
gen_check_unary_mathfn(128, sinh, double);

// CHECK: @check_coshf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_coshf
gen_check_unary_mathfn(128, coshf, float);
// CHECK: @check_cosh
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_cosh
gen_check_unary_mathfn(128, cosh, double);

// CHECK: @check_tanhf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_tanhf
gen_check_unary_mathfn(128, tanhf, float);
// CHECK: @check_tanh
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_tanh
gen_check_unary_mathfn(128, tanh, double);

// CHECK: @check_powf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_powf
gen_check_binary_mathfn(128, powf, float);
// CHECK: @check_pow
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_pow
gen_check_binary_mathfn(128, pow, double);

// CHECK: @check_logf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_logf
gen_check_unary_mathfn(128, logf, float);
// CHECK: @check_log
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_log
gen_check_unary_mathfn(128, log, double);

// CHECK: @check_log10f
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_log10f
gen_check_unary_mathfn(128, log10f, float);
// CHECK: @check_log10
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_log10
gen_check_unary_mathfn(128, log10, double);

// CHECK: @check_log2f
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_log2f
gen_check_unary_mathfn(128, log2f, float);
// CHECK: @check_log2
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_log2
gen_check_unary_mathfn(128, log2, double);

// CHECK: @check_expf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_expf
gen_check_unary_mathfn(128, expf, float);
// CHECK: @check_exp
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_exp
gen_check_unary_mathfn(128, exp, double);

// CHECK: @check_exp2f
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_exp2f
gen_check_unary_mathfn(128, exp2f, float);
// CHECK: @check_exp2
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_exp2
gen_check_unary_mathfn(128, exp2, double);

// CHECK: @check_exp10f
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_exp10f
gen_check_unary_mathfn(128, exp10f, float);
// CHECK: @check_exp10
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_exp10
gen_check_unary_mathfn(128, exp10, double);

// CHECK: @check_fabsf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_fabsf
gen_check_unary_mathfn(128, fabsf, float);
// CHECK: @check_fabs
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_fabs
gen_check_unary_mathfn(128, fabs, double);

// CHECK: @check_copysignf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_copysignf
gen_check_binary_mathfn(128, copysignf, float);
// CHECK: @check_copysign
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_copysign
gen_check_binary_mathfn(128, copysign, double);

// CHECK: @check_floorf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_floorf
gen_check_unary_mathfn(128, floorf, float);
// CHECK: @check_floor
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_floor
gen_check_unary_mathfn(128, floor, double);

// CHECK: @check_ceilf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_ceilf
gen_check_unary_mathfn(128, ceilf, float);
// CHECK: @check_ceil
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_ceil
gen_check_unary_mathfn(128, ceil, double);

// CHECK: @check_truncf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_truncf
gen_check_unary_mathfn(128, truncf, float);
// CHECK: @check_trunc
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_trunc
gen_check_unary_mathfn(128, trunc, double);

// CHECK: @check_rintf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_rintf
gen_check_unary_mathfn(128, rintf, float);
// CHECK: @check_rint
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_rint
gen_check_unary_mathfn(128, rint, double);

// CHECK: @check_nearbyintf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_nearbyintf
gen_check_unary_mathfn(128, nearbyintf, float);
// CHECK: @check_nearbyint
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_nearbyint
gen_check_unary_mathfn(128, nearbyint, double);

// CHECK: @check_roundf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_roundf
gen_check_unary_mathfn(128, roundf, float);
// CHECK: @check_round
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_round
gen_check_unary_mathfn(128, round, double);

// CHECK: @check_roundevenf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_roundevenf
gen_check_unary_mathfn(128, roundevenf, float);
// CHECK: @check_roundeven
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_roundeven
gen_check_unary_mathfn(128, roundeven, double);

#define N 128

// CHECK: @check_ldexpf
// CHECK-COUNT-4: call{{.*}}@ripple_ew_pure{{.*}}_ldexpf
void check_ldexpf(const float x[N], int y[N], float z[N]) {
  ripple_block_t BS = ripple_set_block_shape(0, N);
  int v0 = ripple_id(BS, 0);
  z[v0] = ldexpf(x[v0], y[v0]);
}
// CHECK: @check_ldexp
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure{{.*}}_ldexp
void check_ldexp(const double x[N], int y[N], double z[N]) {
  ripple_block_t BS = ripple_set_block_shape(0, N);
  int v0 = ripple_id(BS, 0);
  z[v0] = ldexp(x[v0], y[v0]);
}
