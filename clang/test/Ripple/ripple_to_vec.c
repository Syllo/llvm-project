// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang -ffreestanding %s -O2 --target=hexagon -fenable-ripple -Wno-psabi -S -emit-llvm -o - | FileCheck %s %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding %s -O2 --target=x86_64-unknown-linux-gnu -fenable-ripple -Wno-psabi -S -emit-llvm -o - | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding %s -O2 --target=aarch64-unknown-linux-gnu -fenable-ripple -Wno-psabi -S -emit-llvm -o - | FileCheck %s %}

#include "ripple_test.h"
typedef int32_t __attribute__((vector_size(128))) Vector_t;
typedef int32_t __attribute__((vector_size(256))) VectorPair_t;
typedef int32_t __attribute__((vector_size(64))) HalfVector;

Vector_t two2one(VectorPair_t pair) {
// CHECK: define dso_local{{.*}}@two2one
  ripple_block_t BS = ripple_set_block_shape(0, 32, 2);
  int32_t x = vec_to_ripple(BS, pair);
  // This "zip" shuffle turned a sequence of pairs into a pair of sequences
  int32_t evens = ripple_slice(x, -1, 0); // 32x1 shape
  int32_t odds = ripple_slice(x, -1, 1); // 32x1 shape
  return ripple_to_vec(32, evens * odds);
}

HalfVector three2one(VectorPair_t pair) {
// CHECK: define dso_local{{.*}}@three2one
  ripple_block_t BS = ripple_set_block_shape(0, 16, 2, 2);
  int32_t x = vec_to_ripple(BS, pair);
  // This "zip" shuffle turned a sequence of pairs into a pair of sequences
  int32_t evens = ripple_slice(x, -1, 0, 0); // 16x1 shape
  int32_t odds = ripple_slice(x, -1, 0, 1); // 16x1 shape
  // A 16-element slice requires a 16-element native vector.
  return ripple_to_vec(16, evens * odds);
}

Vector_t three2two(VectorPair_t pair) {
// CHECK: define dso_local{{.*}}@three2two
  ripple_block_t BS = ripple_set_block_shape(0, 16, 2, 2);
  int32_t x = vec_to_ripple(BS, pair);
  // This "zip" shuffle turned a sequence of pairs into a pair of sequences
  int32_t evens = ripple_slice(x, -1, -1, 0); // 16x2x1 shape
  int32_t odds = ripple_slice(x, -1, -1, 1); // 16x2x1 shape
  return ripple_to_vec(32, evens * odds);
}
