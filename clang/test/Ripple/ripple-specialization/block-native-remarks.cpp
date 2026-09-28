// REQUIRES: hexagon-registered-target
// RUN: %clang_cc1 -I %S/../../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -Rpass=ripple -emit-llvm %s -o %t 2>&1 | FileCheck %s

#include <ripple.h>
using Vec = long long __attribute__((vector_size(16)));

__attribute__((noinline)) static long long seed(ripple_block_t bs, Vec v) {
  return v[0] + ripple_id(bs, 0);
}

void native_argument(Vec v, long long *out) {
  auto bs = ripple_set_block_shape(0, 4);
  out[ripple_id(bs, 0)] = seed(bs, v);
}

// CHECK: remark: function {{.*}} specialized for tensor operands
// CHECK: remark: specialized call to {{.*}}; arg0 unchanged, arg1 unchanged {shape: (unchanged, unchanged) -> Tensor[4]}
