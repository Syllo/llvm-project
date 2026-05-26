// The attribute lowering is target-independent; run on every registered target.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

// A definition gets the "ripple-vector-candidate" string fn attr.
[[clang::ripple_vector_candidate]] void rdef(void) {}
// CHECK: define{{.*}} void @rdef(){{.*}} #[[ATTRS_DEF:[0-9]+]]

// An external declaration also gets it (this is the case headers will use).
[[clang::ripple_vector_candidate]] void rdecl(void);
void caller(void) { rdecl(); }
// CHECK: declare{{.*}} void @rdecl({{.*}}) #[[ATTRS_DECL:[0-9]+]]

// CHECK-DAG: attributes #[[ATTRS_DEF]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
// CHECK-DAG: attributes #[[ATTRS_DECL]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
