// REQUIRES: hexagon-registered-target
// RUN: %clang_cc1 -I %S/../../../lib/Headers/ripple_include -triple hexagon \
// RUN:     -target-cpu hexagonv81 -target-feature +hvxv81 -target-feature +hvx-length128b \
// RUN:     -fenable-ripple -O2 -mllvm -ripple-pad-to-target-simd \
// RUN:     -Rpass-missed=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s

#include <ripple.h>

using Vec = long long __attribute__((vector_size(32)));

// A native vector's type is pinned by the target, so SIMD padding cannot widen
// the conversion to the legal HVX width. That is reported rather than silently
// dropping the padding, because it explains why the surrounding tensor code is
// narrowed back at the boundary.
Vec roundtrip(Vec v) {
  auto bs = ripple_set_block_shape(0, 4);
  return ripple_to_vec<4>(vec_to_ripple(bs, v) + 1);
}

// CHECK: remark: SIMD padding stops at a native vector boundary: this 4-lane conversion is not widened to 16 lanes because the native vector type is fixed by the target [-Rpass-missed=ripple]

// A to.vec-only conversion still gets a remark: the tensor side is padded to the
// legal width and narrowed at the boundary.
void store_block(long long *out) {
  auto bs = ripple_set_block_shape(0, 4);
  Vec v = ripple_to_vec<4>((long long)ripple_id(bs, 0) * 3);
  *(Vec *)out = v;
}

// CHECK: remark: SIMD padding stops at a native vector boundary: this 4-lane conversion is not widened to 16 lanes because the native vector type is fixed by the target [-Rpass-missed=ripple]
