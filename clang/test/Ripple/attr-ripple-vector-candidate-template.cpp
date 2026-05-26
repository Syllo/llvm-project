// ripple_vector_candidate on function templates: implicit instantiation,
// explicit specialization with a body, explicit specialization declaration-only,
// and extern template (explicit instantiation declaration).
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}

typedef float v32f32 __attribute__((__vector_size__(128))) __attribute__((aligned(128)));

// All candidates must appear in @llvm.compiler.used.
// CHECK: @llvm.compiler.used = {{.*}}@_ZL{{[0-9]+}}ripple_ew_pure_add{{.*}}@_Z{{[0-9]+}}ripple_ew_pure_spec_def{{.*}}@_Z{{[0-9]+}}ripple_ew_pure_spec_decl{{.*}}@_Z{{[0-9]+}}ripple_ew_pure_extern

// --- Implicit instantiation ---
// Template with body — simulates a header-only static inline candidate.
template <typename T>
__attribute__((ripple_vector_candidate)) static inline T ripple_ew_pure_add(T a, T b) {
  return a + b;
}
// Two call sites — still only one instantiation in the IR.
void compute(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_add(a, b); }
void compute2(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_add(a, b); }

// CHECK-DAG: define internal {{.*}} @_ZL{{[0-9]+}}ripple_ew_pure_add{{.*}} #[[DEFATTR:[0-9]+]]

// --- Explicit specialization with body ---
template <typename T>
__attribute__((ripple_vector_candidate)) T ripple_ew_pure_spec_def(T a, T b);

template <>
__attribute__((ripple_vector_candidate)) v32f32 ripple_ew_pure_spec_def(v32f32 a, v32f32 b) { return a + b; }

// CHECK-DAG: define {{.*}} @_Z{{[0-9]+}}ripple_ew_pure_spec_def{{.*}} #[[DEFATTR]]

// --- Explicit specialization declaration only ---
// No warning: the declaration-only specialization is never materialised
// (no body, no call site) so SetFunctionAttributes is never reached.
template <typename T>
__attribute__((ripple_vector_candidate)) T ripple_ew_pure_spec_decl(T a, T b);

template <>
__attribute__((ripple_vector_candidate)) v32f32 ripple_ew_pure_spec_decl(v32f32 a, v32f32 b);

// CHECK-DAG: declare {{.*}} @_Z{{[0-9]+}}ripple_ew_pure_spec_decl{{.*}} #[[DCLATTR:[0-9]+]]

// --- extern template (explicit instantiation declaration) ---
// The definition lives in another TU; a call site here forces a declare with
// the attribute so the ripple pass can recognise the candidate.
template <typename T>
__attribute__((ripple_vector_candidate)) T ripple_ew_pure_extern(T a, T b);
extern template v32f32 ripple_ew_pure_extern<v32f32>(v32f32, v32f32);
void compute_extern(v32f32 *out, v32f32 a, v32f32 b) { *out = ripple_ew_pure_extern(a, b); }

// CHECK-DAG: declare {{.*}} @_Z{{[0-9]+}}ripple_ew_pure_extern{{.*}} #[[DCLATTR]]

// CHECK-DAG: attributes #[[DEFATTR]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
// CHECK-DAG: attributes #[[DCLATTR]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
