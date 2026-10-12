// REQUIRES: aarch64-registered-target || hexagon-registered-target || x86-registered-target
// RUN: %if aarch64-registered-target %{ %clang --target=aarch64-none-linux-gnu -c -O2 -fenable-ripple -emit-llvm -S -o - -ffast-math %s | FileCheck %s --implicit-check-not="warning:" %}
// RUN: %if hexagon-registered-target %{ %clang --target=hexagon-unknown-elf -c -O2 -fenable-ripple -emit-llvm -S -o - -ffast-math %s | FileCheck %s --implicit-check-not="warning:" %}
// RUN: %if x86-registered-target %{ %clang --target=x86_64-unknown-linux-gnu -c -O2 -fenable-ripple -emit-llvm -S -o - -ffast-math %s | FileCheck %s --implicit-check-not="warning:" %}

#include "external_library.h"
#include "../ripple_test.h"

#define VEC 0

// We are looking for four calls: two from the loop and two masked tail calls.
// CHECK: ew_cos_external
// CHECK-COUNT-4: call{{.*}}@ripple_{{.*}}cosf
void ew_cos_external(size_t size, float *input, float *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 64);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  size_t i;
  for (i = 0; i + BlockSizeX < size; i += BlockSizeX)
    output[i + BlockX] = cosf(input[i + BlockX]);
  if(i + BlockX < size)
    output[i + BlockX] = cosf(input[i + BlockX]);
}
