// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -emit-llvm %s -o - | FileCheck %s %}

#include "ripple_test.h"

#define VEC 0

// Packed elements are three bytes apart and cannot be represented by i16
// vector lanes. This must remain a gather.
typedef struct __attribute__((packed)) {
    char Padding;
    int16_t Value;
} packed_i16_t;

// CHECK-LABEL: define{{.*}}@multi_window_load_packed_i16
// CHECK: llvm.masked.gather
void multi_window_load_packed_i16(packed_i16_t A[32][32], int16_t *Out) {
    ripple_block_t BS = ripple_set_block_shape(VEC, 4, 4);
    size_t v0 = ripple_id(BS, 0);
    size_t v1 = ripple_id(BS, 1);

    Out[v1 * 4 + v0] = A[v1][v0].Value;
}
