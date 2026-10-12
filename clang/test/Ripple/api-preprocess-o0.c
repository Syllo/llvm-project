// RUN: %clang -O0 -fenable-ripple -S -emit-llvm -mllvm -print-after=ripple-api-preprocess %s -o /dev/null 2>&1 | FileCheck %s --implicit-check-not="warning:" --implicit-check-not="error:"

#include "ripple_test.h"

void check(size_t *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 32, 4);
  size_t id = ripple_id(BS, 1);
  out[id] = ripple_get_block_size(BS, 0);
}

// CHECK: IR Dump After RippleAPIPreprocessPass on check
// CHECK-LABEL: define{{.*}}void @check(
// CHECK: call range([[SIZE:i(32|64)]] 0, 4) [[SIZE]] @llvm.ripple.block.index{{.*}}, !range [[RANGE:![0-9]+]]
// CHECK: store [[SIZE]] 32
// CHECK: ret void
