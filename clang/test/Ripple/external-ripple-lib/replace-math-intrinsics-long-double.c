// REQUIRES: x86-registered-target
// RUN: %clang --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm -S -o - -ffast-math %s | FileCheck %s --implicit-check-not="warning:"

#include "../ripple_test.h"
#include <ripple_math.h>

// The old test loaded these candidates from external_library.c.  Keep the
// test self-contained now that Ripple runtime libraries are header-only.
typedef long double f128t8 __attribute__((vector_size(128)));
typedef int32_t i32t8 __attribute__((vector_size(32)));
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))

#define DECLARE_UNARY_MATH_CANDIDATE(NAME)                                    \
  extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_##NAME##l(f128t8);
#define DECLARE_BINARY_MATH_CANDIDATE(NAME)                                   \
  extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_##NAME##l(f128t8,      \
                                                                  f128t8);

DECLARE_UNARY_MATH_CANDIDATE(sqrt)
DECLARE_UNARY_MATH_CANDIDATE(asin)
DECLARE_UNARY_MATH_CANDIDATE(acos)
DECLARE_UNARY_MATH_CANDIDATE(atan)
DECLARE_UNARY_MATH_CANDIDATE(sin)
DECLARE_UNARY_MATH_CANDIDATE(cos)
DECLARE_UNARY_MATH_CANDIDATE(tan)
DECLARE_UNARY_MATH_CANDIDATE(sinh)
DECLARE_UNARY_MATH_CANDIDATE(cosh)
DECLARE_UNARY_MATH_CANDIDATE(tanh)
DECLARE_UNARY_MATH_CANDIDATE(log)
DECLARE_UNARY_MATH_CANDIDATE(log10)
DECLARE_UNARY_MATH_CANDIDATE(log2)
DECLARE_UNARY_MATH_CANDIDATE(exp)
DECLARE_UNARY_MATH_CANDIDATE(exp2)
DECLARE_UNARY_MATH_CANDIDATE(exp10)
DECLARE_UNARY_MATH_CANDIDATE(fabs)
DECLARE_UNARY_MATH_CANDIDATE(floor)
DECLARE_UNARY_MATH_CANDIDATE(ceil)
DECLARE_UNARY_MATH_CANDIDATE(trunc)
DECLARE_UNARY_MATH_CANDIDATE(rint)
DECLARE_UNARY_MATH_CANDIDATE(nearbyint)
DECLARE_UNARY_MATH_CANDIDATE(round)
DECLARE_UNARY_MATH_CANDIDATE(roundeven)

DECLARE_BINARY_MATH_CANDIDATE(atan2)
DECLARE_BINARY_MATH_CANDIDATE(pow)
DECLARE_BINARY_MATH_CANDIDATE(copysign)

extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_ldexpl(f128t8, i32t8);

#undef DECLARE_UNARY_MATH_CANDIDATE
#undef DECLARE_BINARY_MATH_CANDIDATE

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
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_sqrtl
gen_check_unary_mathfn(128, sqrtl, long double);

// CHECK: @check_asinl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_asinl
gen_check_unary_mathfn(128, asinl, long double);

// CHECK: @check_acosl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_acosl
gen_check_unary_mathfn(128, acosl, long double);

// CHECK: @check_atanl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_atanl
gen_check_unary_mathfn(128, atanl, long double);

// CHECK: @check_atan2l
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_atan2l
gen_check_binary_mathfn(128, atan2l, long double);

// CHECK: @check_sinl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_sinl
gen_check_unary_mathfn(128, sinl, long double);

// CHECK: @check_cosl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_cosl
gen_check_unary_mathfn(128, cosl, long double);

// CHECK: @check_tanl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_tanl
gen_check_unary_mathfn(128, tanl, long double);

// CHECK: @check_sinhl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_sinhl
gen_check_unary_mathfn(128, sinhl, long double);

// CHECK: @check_coshl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_coshl
gen_check_unary_mathfn(128, coshl, long double);

// CHECK: @check_tanhl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_tanhl
gen_check_unary_mathfn(128, tanhl, long double);

// CHECK: @check_powl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_powl
gen_check_binary_mathfn(128, powl, long double);

// CHECK: @check_logl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_logl
gen_check_unary_mathfn(128, logl, long double);

// CHECK: @check_log10l
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_log10l
gen_check_unary_mathfn(128, log10l, long double);

// CHECK: @check_log2l
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_log2l
gen_check_unary_mathfn(128, log2l, long double);

// CHECK: @check_expl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_expl
gen_check_unary_mathfn(128, expl, long double);

// CHECK: @check_exp2l
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_exp2l
gen_check_unary_mathfn(128, exp2l, long double);

// CHECK: @check_exp10l
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_exp10l
gen_check_unary_mathfn(128, exp10l, long double);

// CHECK: @check_fabsl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_fabsl
gen_check_unary_mathfn(128, fabsl, long double);

// CHECK: @check_copysignl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_copysignl
gen_check_binary_mathfn(128, copysignl, long double);

// CHECK: @check_floorl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_floorl
gen_check_unary_mathfn(128, floorl, long double);

// CHECK: @check_ceill
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_ceill
gen_check_unary_mathfn(128, ceill, long double);

// CHECK: @check_truncl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_truncl
gen_check_unary_mathfn(128, truncl, long double);

// CHECK: @check_rintl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_rintl
gen_check_unary_mathfn(128, rintl, long double);

// CHECK: @check_nearbyintl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_nearbyintl
gen_check_unary_mathfn(128, nearbyintl, long double);

// CHECK: @check_roundl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_roundl
gen_check_unary_mathfn(128, roundl, long double);

// CHECK: @check_roundevenl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_roundevenl
gen_check_unary_mathfn(128, roundevenl, long double);

#define N 128

// CHECK: @check_ldexpl
// CHECK-COUNT-16: call{{.*}}@ripple_ew_pure_ldexpl
void check_ldexpl(const long double x[N], int y[N], long double z[N]) {
  ripple_block_t BS = ripple_set_block_shape(0, N);
  int v0 = ripple_id(BS, 0);
  z[v0] = ldexpl(x[v0], y[v0]);
}
