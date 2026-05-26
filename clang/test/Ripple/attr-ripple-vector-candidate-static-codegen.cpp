// C++ ripple vector candidates with internal linkage (static functions and
// anonymous-namespace functions) must be emitted and pinned against pre-ripple
// GlobalDCE even when they have no call sites, so the ripple pass can discover
// them. The attribute lowering and used-list pinning are target-independent,
// so run on every target we care about.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}

// All candidates are pinned so pre-ripple GlobalDCE cannot drop them. The
// used-list global is emitted before the function bodies.
// CHECK: @llvm.compiler.used = {{.*}}@_ZN12_GLOBAL__N_18anon_defEv{{.*}}@_ZL5s_defv{{.*}}@_ZL6si_defv{{.*}}@_ZL7fwd_defv{{.*}}@_ZL12fwd_both_defv

// An anonymous-namespace definition gets internal linkage and the attribute.
namespace {
__attribute__((ripple_vector_candidate)) void anon_def(void) {}
}
// CHECK-DAG: define internal void @_ZN12_GLOBAL__N_18anon_defEv() #[[ATTRS:[0-9]+]]

// A static definition gets internal linkage and the attribute.
__attribute__((ripple_vector_candidate)) static void s_def(void) {}
// CHECK-DAG: define internal void @_ZL5s_defv() #[[ATTRS]]

// A static inline definition is likewise emitted with internal linkage.
__attribute__((ripple_vector_candidate)) static inline void si_def(void) {}
// CHECK-DAG: define internal void @_ZL6si_defv() #[[ATTRS]]

// The attribute on a forward declaration propagates to the emitted definition:
// fwd_def is declared with the attribute, called from fwd_caller, then defined
// without repeating the attribute. fwd_caller itself must not get the attribute.
__attribute__((ripple_vector_candidate)) static void fwd_def(void);
static void fwd_caller(void) { fwd_def(); }
static void fwd_def(void) {}
void use_fwd_caller(void) { fwd_caller(); }
// CHECK-DAG: define internal void @_ZL7fwd_defv() #[[ATTRS]]
// CHECK-DAG: define internal void @_ZL10fwd_callerv() #[[NO_ATTRS:[0-9]+]]

// The attribute on both the forward declaration and the definition is also fine.
// fwd_both_caller likewise must not get the attribute.
__attribute__((ripple_vector_candidate)) static void fwd_both_def(void);
static void fwd_both_caller(void) { fwd_both_def(); }
__attribute__((ripple_vector_candidate)) static void fwd_both_def(void) {}
void use_fwd_both_caller(void) { fwd_both_caller(); }
// CHECK-DAG: define internal void @_ZL12fwd_both_defv() #[[ATTRS]]
// CHECK-DAG: define internal void @_ZL15fwd_both_callerv() #[[NO_ATTRS]]

// CHECK: attributes #[[ATTRS]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
// CHECK: attributes #[[NO_ATTRS]] = {
// CHECK-NOT: "ripple-vector-candidate"
