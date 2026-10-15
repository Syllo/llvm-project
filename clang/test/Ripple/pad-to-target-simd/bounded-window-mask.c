// REQUIRES: hexagon-registered-target
// RUN: %clang -ffreestanding -S --target=hexagon-unknown-elf -mhvx -mv81 \
// RUN:   -mhvx-length=128B -O2 -fenable-ripple \
// RUN:   -mllvm -ripple-pad-to-target-simd -emit-llvm %s -o %t.ll

#include "../ripple_test.h"

// A bounded single-window load with a non-injective index creates a mask
// whose padding can fold to a constant.
void padded_bounded_window_mask(int16_t *a, int16_t *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 60);
  size_t v0 = ripple_id(BS, 0);
  out[v0] = a[v0 / 2];
}
