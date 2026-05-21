#pragma once

#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif

typedef _Float16 f16t64 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));
typedef float f32t32 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));
typedef double f64t16 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));
typedef long double f128t8 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));
typedef __INT32_TYPE__ i32t64 __attribute__((__vector_size__(256)))
__attribute__((aligned(256)));
typedef __INT32_TYPE__ i32t32 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));
typedef __INT32_TYPE__ i32t16 __attribute__((__vector_size__(64)))
__attribute__((aligned(64)));
typedef __INT32_TYPE__ i32t8 __attribute__((__vector_size__(32)))
__attribute__((aligned(32)));

typedef char i1t64 __attribute__((__vector_size__(64)))
__attribute__((aligned(64)));

extern _Float16 cosf16(_Float16);
extern _Float16 add_and_double(_Float16, _Float16);
extern _Float16 sinf16(_Float16);
extern _Float16 add_and_half(_Float16, _Float16);
extern _Float16 add_and_half_ptr(_Float16, _Float16*);
extern _Float16 add_and_half_ptr_has_no_mask_version(_Float16, _Float16*);
extern _Float16 non_pure_ew_separate_mask(_Float16);
extern float cosf(float);
extern _Float16 mysinf16(_Float16);
extern void side_effect_no_return(float, float);

extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_add_and_double(f16t64 A,
                                                               f16t64 B);
extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_add_and_half(f16t64 A, _Float16 B);
extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_add_and_half_ptr(f16t64 A,
                                                              _Float16 *B);
extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_mask_add_and_half_ptr(f16t64 A, _Float16 *B, i1t64 Mask);
extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_add_and_half_ptr_has_no_mask_version(f16t64 A, _Float16 *B);
extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_ew_non_pure_ew_separate_mask(f16t64 A);
extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_ew_mask_non_pure_ew_separate_mask(f16t64 A, i1t64 Mask);
extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_doublify(f32t32 A);
extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_mysinf16(f16t64 A);

#define DECLARE_UNARY_MATH_TEST_FUN(name)                                      \
  extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_pure_##name##f16(            \
      f16t64 Vals);                                                            \
  extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_pure_##name##f(f32t32 Vals); \
  extern RIPPLE_VECTOR_CANDIDATE f64t16 ripple_ew_pure_##name(f64t16 Vals);    \
  extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_##name##l(f128t8 Vals);

DECLARE_UNARY_MATH_TEST_FUN(sqrt)
DECLARE_UNARY_MATH_TEST_FUN(asin)
DECLARE_UNARY_MATH_TEST_FUN(acos)
DECLARE_UNARY_MATH_TEST_FUN(atan)
DECLARE_UNARY_MATH_TEST_FUN(sin)
DECLARE_UNARY_MATH_TEST_FUN(cos)
DECLARE_UNARY_MATH_TEST_FUN(tan)
DECLARE_UNARY_MATH_TEST_FUN(sinh)
DECLARE_UNARY_MATH_TEST_FUN(cosh)
DECLARE_UNARY_MATH_TEST_FUN(tanh)
DECLARE_UNARY_MATH_TEST_FUN(log)
DECLARE_UNARY_MATH_TEST_FUN(log10)
DECLARE_UNARY_MATH_TEST_FUN(log2)
DECLARE_UNARY_MATH_TEST_FUN(exp)
DECLARE_UNARY_MATH_TEST_FUN(exp2)
DECLARE_UNARY_MATH_TEST_FUN(exp10)
DECLARE_UNARY_MATH_TEST_FUN(fabs)
DECLARE_UNARY_MATH_TEST_FUN(floor)
DECLARE_UNARY_MATH_TEST_FUN(ceil)
DECLARE_UNARY_MATH_TEST_FUN(trunc)
DECLARE_UNARY_MATH_TEST_FUN(rint)
DECLARE_UNARY_MATH_TEST_FUN(nearbyint)
DECLARE_UNARY_MATH_TEST_FUN(round)
DECLARE_UNARY_MATH_TEST_FUN(roundeven)

#undef DECLARE_UNARY_MATH_TEST_FUN

#define DECLARE_BINARY_MATH_TEST_FUN(name)                                     \
  extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_pure_##name##f16(            \
      f16t64 Vals, f16t64 Vals2);                                              \
  extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_pure_##name##f(              \
      f32t32 Vals, f32t32 Vals2);                                              \
  extern RIPPLE_VECTOR_CANDIDATE f64t16 ripple_ew_pure_##name(                 \
      f64t16 Vals, f64t16 Vals2);                                              \
  extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_##name##l(              \
      f128t8 Vals, f128t8 Vals2);

DECLARE_BINARY_MATH_TEST_FUN(atan2)
DECLARE_BINARY_MATH_TEST_FUN(pow)
DECLARE_BINARY_MATH_TEST_FUN(copysign)

#undef DECLARE_BINARY_MATH_TEST_FUN

extern RIPPLE_VECTOR_CANDIDATE void
ripple_ew_pure_sincosf16(f16t64 Vals, f16t64 *sin, f16t64 *cos);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_ew_pure_sincosf(f32t32 Vals, f32t32 *sin, f32t32 *cos);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_ew_pure_sincos(f64t16 Vals, f64t16 *sin, f64t16 *cos);
extern RIPPLE_VECTOR_CANDIDATE void
ripple_ew_pure_sincosl(f128t8 Vals, f128t8 *sin, f128t8 *cos);

extern RIPPLE_VECTOR_CANDIDATE f16t64
ripple_ew_pure_modff16(f16t64 Vals, f16t64 *FractionalPart);
extern RIPPLE_VECTOR_CANDIDATE f32t32
ripple_ew_pure_modff(f32t32 Vals, f32t32 *FractionalPart);
extern RIPPLE_VECTOR_CANDIDATE f64t16
ripple_ew_pure_modf(f64t16 Vals, f64t16 *FractionalPart);
extern RIPPLE_VECTOR_CANDIDATE f128t8
ripple_ew_pure_modfl(f128t8 Vals, f128t8 *FractionalPart);

extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_pure_ldexpf16(f16t64 Vals,
                                                              i32t64 Exp);
extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_pure_ldexpf(f32t32 Vals,
                                                            i32t32 Exp);
extern RIPPLE_VECTOR_CANDIDATE f64t16 ripple_ew_pure_ldexp(f64t16 Vals,
                                                           i32t16 Exp);
extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_ldexpl(f128t8 Vals,
                                                            i32t8 Exp);

extern RIPPLE_VECTOR_CANDIDATE f16t64 ripple_ew_pure_frexpf16(f16t64 Vals,
                                                              i32t64 *Exp);
extern RIPPLE_VECTOR_CANDIDATE f32t32 ripple_ew_pure_frexpf(f32t32 Vals,
                                                            i32t32 *Exp);
extern RIPPLE_VECTOR_CANDIDATE f64t16 ripple_ew_pure_frexp(f64t16 Vals,
                                                           i32t16 *Exp);
extern RIPPLE_VECTOR_CANDIDATE f128t8 ripple_ew_pure_frexpl(f128t8 Vals,
                                                            i32t8 *Exp);

extern RIPPLE_VECTOR_CANDIDATE int
ripple_ew_pure_this_is_not_really_a_ripple_function(int Val);
extern RIPPLE_VECTOR_CANDIDATE int
ripple_ew_this_is_not_really_a_ripple_function2(int Val);
extern RIPPLE_VECTOR_CANDIDATE int
ripple_this_is_not_really_a_ripple_function3(int Val);
extern RIPPLE_VECTOR_CANDIDATE int
ripple_mask_this_is_not_really_a_ripple_function3(int Val);

extern RIPPLE_VECTOR_CANDIDATE void
ripple_ew_side_effect_no_return(f32t32 f1, f32t32 f2);
