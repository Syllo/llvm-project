// 'ripple_vector_candidate' must warn when -fenable-ripple is not set for
// function templates: implicit instantiation, explicit specialization with a
// body, and extern template (explicit instantiation declaration).
// Must not lower to the "ripple-vector-candidate" string fn attr or pin the
// function via llvm.compiler.used in that case.
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm %s -o - -verify -Wno-psabi 2>&1 | FileCheck %s

typedef float v32f32 __attribute__((__vector_size__(128))) __attribute__((aligned(128)));

// --- Implicit instantiation ---
// Without -fenable-ripple the warning fires once at the template definition
// regardless of how many call sites instantiate it.
template <typename T>
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) static inline T ripple_ew_pure_add(T a, T b) {
  return a + b;
}
// Two call sites — still one warning above.
void compute(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_add(a, b); }
void compute2(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_add(a, b); }

// --- Explicit specialization with body ---
// The warning fires at both the primary template declaration and the
// specialization definition since each spells the attribute.
template <typename T>
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) T ripple_ew_pure_spec_def(T a, T b);

template <>
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) v32f32 ripple_ew_pure_spec_def(v32f32 a, v32f32 b) { return a + b; }

// --- extern template (explicit instantiation declaration) ---
// The definition lives in another TU; a call site here forces a declare.
// The warning fires at the primary template when the call site materialises it.
template <typename T>
// expected-warning@+1 {{'ripple_vector_candidate' attribute has no effect without '-fenable-ripple'}}
__attribute__((ripple_vector_candidate)) T ripple_ew_pure_extern(T a, T b);
extern template v32f32 ripple_ew_pure_extern<v32f32>(v32f32, v32f32);
void compute_extern(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_extern(a, b); }

// CHECK-NOT: "ripple-vector-candidate"
// CHECK-NOT: llvm.compiler.used
// CHECK-NOT: llvm.used
