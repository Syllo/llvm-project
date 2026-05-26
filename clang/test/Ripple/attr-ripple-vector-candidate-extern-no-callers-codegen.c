// An extern ripple_vector_candidate declaration with no call sites must still
// land in the IR so the Ripple pass can discover it as a candidate.  This
// exercises the EmitGlobal guard: without the RippleVectorCandidateAttr check
// the early-return for bodyless, unreferenced declarations would silently drop
// the function.
//
// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

// No caller in this TU — the declaration must still be materialised.
[[clang::ripple_vector_candidate]] void rdecl_no_callers(void);

// CHECK: declare{{.*}} void @rdecl_no_callers({{.*}}) #[[ATTRS:[0-9]+]]
// CHECK: attributes #[[ATTRS]] = {{{.*}}"ripple-vector-candidate"{{.*}}}
