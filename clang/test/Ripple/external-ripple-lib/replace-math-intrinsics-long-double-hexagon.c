// REQUIRES: hexagon-registered-target
// RUN: %clang --target=hexagon-unknown-elf -O2 -fenable-ripple -emit-llvm -S -o - %s | FileCheck %s --implicit-check-not="warning:"

#include "../ripple_test.h"
#include "external_library.h"
#include <ripple_math.h>

// hexagon uses double == long double

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


// CHECK: @check_sqrtl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_sqrt
gen_check_unary_mathfn(128, sqrtl, long double);

// CHECK: @check_asinl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_asin
gen_check_unary_mathfn(128, asinl, long double);

// CHECK: @check_acosl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_acos
gen_check_unary_mathfn(128, acosl, long double);

// CHECK: @check_atanl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_atan
gen_check_unary_mathfn(128, atanl, long double);

// CHECK: @check_atan2l
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_atan2
gen_check_binary_mathfn(128, atan2l, long double);

// CHECK: @check_sinl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_sin
gen_check_unary_mathfn(128, sinl, long double);

// CHECK: @check_cosl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_cos
gen_check_unary_mathfn(128, cosl, long double);

// CHECK: @check_tanl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_tan
gen_check_unary_mathfn(128, tanl, long double);

// CHECK: @check_sinhl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_sinh
gen_check_unary_mathfn(128, sinhl, long double);

// CHECK: @check_coshl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_cosh
gen_check_unary_mathfn(128, coshl, long double);

// CHECK: @check_tanhl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_tanh
gen_check_unary_mathfn(128, tanhl, long double);

// CHECK: @check_powl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_pow
gen_check_binary_mathfn(128, powl, long double);

// CHECK: @check_logl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_log
gen_check_unary_mathfn(128, logl, long double);

// CHECK: @check_log10l
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_log10
gen_check_unary_mathfn(128, log10l, long double);

// CHECK: @check_log2l
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_log2
gen_check_unary_mathfn(128, log2l, long double);

// CHECK: @check_expl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_exp
gen_check_unary_mathfn(128, expl, long double);

// CHECK: @check_exp2l
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_exp2
gen_check_unary_mathfn(128, exp2l, long double);

// CHECK: @check_exp10l
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_exp10
gen_check_unary_mathfn(128, exp10l, long double);

// CHECK: @check_fabsl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_fabs
gen_check_unary_mathfn(128, fabsl, long double);

// CHECK: @check_copysignl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_copysign
gen_check_binary_mathfn(128, copysignl, long double);

// CHECK: @check_floorl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_floor
gen_check_unary_mathfn(128, floorl, long double);

// CHECK: @check_ceill
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_ceil
gen_check_unary_mathfn(128, ceill, long double);

// CHECK: @check_truncl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_trunc
gen_check_unary_mathfn(128, truncl, long double);

// CHECK: @check_rintl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_rint
gen_check_unary_mathfn(128, rintl, long double);

// CHECK: @check_nearbyintl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_nearbyint
gen_check_unary_mathfn(128, nearbyintl, long double);

// CHECK: @check_roundl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_round
gen_check_unary_mathfn(128, roundl, long double);

// CHECK: @check_roundevenl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_roundeven
gen_check_unary_mathfn(128, roundevenl, long double);

#define N 128

// CHECK: @check_ldexpl
// CHECK-COUNT-8: call{{.*}}@ripple_ew_pure_{{.*}}_ldexp
void check_ldexpl(const long double x[N], int y[N], long double z[N]) {
  ripple_block_t BS = ripple_set_block_shape(0, N);
  int v0 = ripple_id(BS, 0);
  z[v0] = ldexpl(x[v0], y[v0]);
}

