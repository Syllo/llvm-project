// Ripple vector candidates with internal linkage (C static functions) must be
// emitted and pinned against pre-ripple GlobalDCE even when they have no call
// sites, so the ripple pass can discover them. A bodyless static declaration
// is likewise materialized so the candidate is visible. The attribute lowering
// and used-list pinning are target-independent, so run on every target we care
// about.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s %}

// All candidates are pinned so pre-ripple GlobalDCE cannot drop them. The
// used-list global is emitted before the function bodies.
// CHECK: @llvm.compiler.used = {{.*}}@s_def{{.*}}@si_def{{.*}}@s_decl

// A static definition is emitted with internal linkage and the attribute.
__attribute__((ripple_vector_candidate)) static void s_def(void) {}
// CHECK-DAG: define internal void @s_def() #[[ATTRS:[0-9]+]]

// A static inline definition is likewise emitted with internal linkage.
__attribute__((ripple_vector_candidate)) static inline void si_def(void) {}
// CHECK-DAG: define internal void @si_def() #[[ATTRS]]

// A bodyless static declaration is materialized as a declaration.
__attribute__((ripple_vector_candidate)) static void s_decl(void);
// CHECK-DAG: declare void @s_decl() #{{[0-9]+}}

// CHECK: attributes #[[ATTRS]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
