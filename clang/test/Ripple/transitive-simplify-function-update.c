// RUN: %clang -fenable-ripple -O3 -S -emit-llvm %s -o - 2>&1 | FileCheck %s

// Regression test: simplifyFunction folds an outer-loop branch condition to
// a constant, leaving the branch's inferred Tensor[N] shape stale w.r.t. its
// scalar operand. Without transitive shape/linear-series re-propagation, the
// vectorizer would assert in genVectorInstructions on `ConditionShape ==
// toShape`. Just check that compilation succeeds without errors or warnings.

// CHECK-NOT: error:
// CHECK-NOT: warning:
// CHECK: define {{.*}}@mult_with_col

#include "ripple_test.h"


void mult_with_col(float A[64][32], const float col[64]) {
  ripple_block_t BS = ripple_set_block_shape(0, 32);
  for (int i = 0; i < 64; i++) {
    float scale = col[i];
    ripple_parallel(BS, 0);
    for (int j = 0; j < 32; j++) {
      A[i][j] *= scale;
    }
  }
}
