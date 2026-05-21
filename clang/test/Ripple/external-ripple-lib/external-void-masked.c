// REQUIRES: target-x86_64 || target=hexagon{{.*}}
// RUN: %clang -g -O2 -fenable-ripple -emit-llvm -S %s -o - | FileCheck %s

#include "../ripple_test.h"

#define VEC 0

typedef int32_t i32t32 __attribute__((__vector_size__(128)))
__attribute__((aligned(128)));

extern void f(int32_t *, int32_t *, int32_t);

extern __attribute__((ripple_vector_candidate)) void
ripple_f(int32_t *a, int32_t *b, i32t32 c);
extern __attribute__((ripple_vector_candidate)) void
ripple_mask_f(int32_t *a, int32_t *b, i32t32 c, i32t32 d);

// CHECK: @test
// CHECK: call void @ripple_f
// CHECK: call void @ripple_mask_f
void test(size_t size, int32_t *input, int32_t *output) {
  ripple_block_t BS = ripple_set_block_shape(VEC, 32);
  size_t BlockX = ripple_id(BS, 0);
  size_t BlockSizeX = ripple_get_block_size(BS, 0);
  ripple_parallel(BS, 0);
  for (int32_t i = 0; i < size; i++)
    f(input, output, i);
}
