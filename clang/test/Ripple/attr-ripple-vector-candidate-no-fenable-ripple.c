// 'ripple_vector_candidate' must warn when -fenable-ripple is not set, and
// must not lower to the "ripple-vector-candidate" string fn attr or pin the
// function via llvm.compiler.used in that case.
// RUN: %clang_cc1 -emit-llvm %s -o - -verify 2>&1 | FileCheck %s

// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
[[clang::ripple_vector_candidate]] void f(void) {}

// A declaration without a body warns at the declaration site.
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) void decl_only(void);

// A forward declaration followed by a definition warns once per spelling site.
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) void g(void);
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) void g(void) {}

// CHECK-NOT: "ripple-vector-candidate"
// CHECK-NOT: llvm.compiler.used
// CHECK-NOT: llvm.used
