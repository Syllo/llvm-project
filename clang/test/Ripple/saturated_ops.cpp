// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %clangxx -S -fenable-ripple -O1 -emit-llvm %s -o - 2>&1 | FileCheck %s --implicit-check-not="warning:" --implicit-check-not="error:"

#include "ripple_test.h"

void foo() {
    ripple_block_t BS = ((ripple_block_t)__builtin_ripple_set_shape( (0), (128), 1, 1, 1, 1, 1, 1, 1, 1, 1));
    uint8_t x = __builtin_ripple_get_index((BS), (0));
    uint8_t y = 2 * __builtin_ripple_get_index((BS), (0));
    uint8_t ref_ = 0;
    uint8_t got_ = ripple_sub_sat(x, y);

    got_ = ripple_add_sat(x, y);
    got_ = ripple_shl_sat(x, y);
}

