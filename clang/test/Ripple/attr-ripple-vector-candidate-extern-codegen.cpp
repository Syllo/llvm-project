// The attribute lowering is target-independent; run on every registered target.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
__attribute__((ripple_vector_candidate)) void gnu_spelling(void) {}
[[clang::ripple_vector_candidate]] void cxx11_spelling(void) {}
// CHECK-DAG: define {{.*}} void @{{.*}}gnu_spelling{{.*}} #[[A:[0-9]+]]
// CHECK-DAG: define {{.*}} void @{{.*}}cxx11_spelling{{.*}} #[[A]]
// CHECK: attributes #[[A]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
