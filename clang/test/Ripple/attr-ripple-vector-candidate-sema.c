// RUN: %clang_cc1 -fsyntax-only -verify -fenable-ripple %s
// RUN: %clang_cc1 -fsyntax-only -verify -fenable-ripple -std=c17 %s
// RUN: %clang_cc1 -fsyntax-only -verify -fenable-ripple -std=c23 %s

__attribute__((ripple_vector_candidate)) void ok_def(void) {}
__attribute__((ripple_vector_candidate)) void ok_decl(void);

// expected-error@+1 {{'ripple_vector_candidate' attribute only applies to functions}}
int x __attribute__((ripple_vector_candidate));

// expected-error@+1 {{'ripple_vector_candidate' attribute takes no arguments}}
__attribute__((ripple_vector_candidate(1))) void bad_args(void);

struct S {
  // expected-error@+1 {{'ripple_vector_candidate' attribute only applies to functions}}
  void (*fp)(void) __attribute__((ripple_vector_candidate));
};

#if __STDC_VERSION__ >= 202311L
// C23 [[clang::ripple_vector_candidate]] spelling.
[[clang::ripple_vector_candidate]] void ok_c23_def(void) {}
[[clang::ripple_vector_candidate]] void ok_c23_decl(void);

// expected-error@+1 {{'clang::ripple_vector_candidate' attribute only applies to functions}}
[[clang::ripple_vector_candidate]] int y;

// expected-error@+1 {{'clang::ripple_vector_candidate' attribute takes no arguments}}
[[clang::ripple_vector_candidate(1)]] void bad_args_c23(void);

struct T {
  // expected-error@+1 {{'clang::ripple_vector_candidate' attribute only applies to functions}}
  [[clang::ripple_vector_candidate]] void (*fp2)(void);
};
#endif
