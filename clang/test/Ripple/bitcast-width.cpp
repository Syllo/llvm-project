// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ not %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s %}
// RUN: %if x86-registered-target %{ not %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ not %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s %}

#include <ripple.h>
using Vec = long long __attribute__((vector_size(256)));

Vec wrong_width(Vec v) {
  auto bs = ripple_set_block_shape(0, 16);
  auto x = vec_to_ripple(bs, v);
  return ripple_to_vec<32>(x);
}
// CHECK: Ripple fixed-vector conversion width mismatch: native vector has 2048 bits
// CHECK-SAME: has 1024 bits
