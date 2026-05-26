// ripple_vector_candidate declarations with incomplete return or parameter
// types must not crash during CodeGen and must not be materialised into the
// IR (there is no representable LLVM type for an incomplete struct).
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

struct Incomplete;
__attribute__((ripple_vector_candidate)) struct Incomplete incomplete_ret(void);
__attribute__((ripple_vector_candidate)) void incomplete_param(struct Incomplete p);

// Neither function must appear in the IR at all.
// CHECK-NOT: @incomplete_ret
// CHECK-NOT: @incomplete_param
// CHECK-NOT: "ripple-vector-candidate"
// CHECK-NOT: llvm.compiler.used
