// Tests the LTO cross-TU scenario: the ripple_vector_candidate definition lives
// in a separate translation unit. Both TUs are compiled with -flto/-flto=thin
// -c, which embeds LTO bitcode in .o files. The ripple pass runs at pre-link
// time during each TU's compilation; the annotated extern declaration in
// caller.c is sufficient for the substitution to happen before any link step.
// llvm-link then verifies that the two bitcode modules are well-formed and
// compatible. Both full LTO and ThinLTO variants are exercised.
//
// REQUIRES: hexagon-registered-target || x86-registered-target
// RUN: rm -rf %t && mkdir %t && split-file %s %t
//
// -- Full LTO: Hexagon --
// RUN: %if hexagon-registered-target %{ \
// RUN:   %clang -O2 -fenable-ripple -flto -c --target=hexagon -mhvx -mv79 \
// RUN:     %t/candidate.c -o %t/cand_hx.o && \
// RUN:   %clang -O2 -fenable-ripple -flto -c --target=hexagon -mhvx -mv79 \
// RUN:     %t/caller.c -o %t/call_hx.o && \
// RUN:   llvm-link %t/cand_hx.o %t/call_hx.o -o - | llvm-dis | FileCheck %s %}
//
// -- Full LTO: x86 --
// RUN: %if x86-registered-target %{ \
// RUN:   %clang -O2 -fenable-ripple -flto -c --target=x86_64-unknown-linux-gnu \
// RUN:     %t/candidate.c -o %t/cand_x86.o && \
// RUN:   %clang -O2 -fenable-ripple -flto -c --target=x86_64-unknown-linux-gnu \
// RUN:     %t/caller.c -o %t/call_x86.o && \
// RUN:   llvm-link %t/cand_x86.o %t/call_x86.o -o - | llvm-dis | FileCheck %s %}
//
// -- ThinLTO: Hexagon --
// RUN: %if hexagon-registered-target %{ \
// RUN:   %clang -O2 -fenable-ripple -flto=thin -c --target=hexagon -mhvx -mv79 \
// RUN:     %t/candidate.c -o %t/cand_thin_hx.o && \
// RUN:   %clang -O2 -fenable-ripple -flto=thin -c --target=hexagon -mhvx -mv79 \
// RUN:     %t/caller.c -o %t/call_thin_hx.o && \
// RUN:   llvm-link %t/cand_thin_hx.o %t/call_thin_hx.o -o - | llvm-dis | FileCheck %s %}
//
// -- ThinLTO: x86 --
// RUN: %if x86-registered-target %{ \
// RUN:   %clang -O2 -fenable-ripple -flto=thin -c --target=x86_64-unknown-linux-gnu \
// RUN:     %t/candidate.c -o %t/cand_thin_x86.o && \
// RUN:   %clang -O2 -fenable-ripple -flto=thin -c --target=x86_64-unknown-linux-gnu \
// RUN:     %t/caller.c -o %t/call_thin_x86.o && \
// RUN:   llvm-link %t/cand_thin_x86.o %t/call_thin_x86.o -o - | llvm-dis | FileCheck %s %}

// CHECK-LABEL: define {{.*}} @compute(
// CHECK-NOT:   call float @myaddf(
// CHECK:       call {{.*}} @ripple_ew_pure_myaddf(
// CHECK-NOT:   call float @myaddf(

//--- candidate.c
// The ripple_vector_candidate definition lives in its own TU.
typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));

__attribute__((ripple_vector_candidate))
f32t32 ripple_ew_pure_myaddf(f32t32 a, f32t32 b) {
  return a + b;
}

//--- caller.c
// The caller TU uses the scalar myaddf inside a ripple block and carries an
// extern declaration of the vector candidate.
#include <ripple.h>

typedef float f32t32 __attribute__((__vector_size__(128)))
    __attribute__((aligned(128)));

extern float myaddf(float a, float b);

__attribute__((ripple_vector_candidate))
extern f32t32 ripple_ew_pure_myaddf(f32t32 a, f32t32 b);

void compute(float *out, float *a, float *b) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  __SIZE_TYPE__ i = ripple_id(BS, 0);
  out[i] = myaddf(a[i], b[i]);
}
