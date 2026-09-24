// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s --check-prefix=FRONTEND %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s --check-prefix=FRONTEND %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -disable-llvm-passes -emit-llvm %s -o - | FileCheck %s --check-prefix=FRONTEND %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=LOWERED-HEX --implicit-check-not=alloca --implicit-check-not=llvm.ripple.bitcast --implicit-check-not=llvm.masked %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=LOWERED-GENERIC --implicit-check-not=llvm.ripple.bitcast --implicit-check-not=llvm.masked %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=LOWERED-GENERIC --implicit-check-not=llvm.ripple.bitcast --implicit-check-not=llvm.masked %}

#include <ripple.h>

using Vec = long long __attribute__((vector_size(256)));

extern "C" Vec round_trip(Vec v) {
  auto bs = ripple_set_block_shape(0, 32);
  return ripple_to_vec<32>(vec_to_ripple(bs, v));
}
// FRONTEND: call i64 @llvm.ripple.from.fixed.vector.v32i64
// FRONTEND: call <32 x i64> @llvm.ripple.to.fixed.vector.v32i64
// LOWERED-HEX-LABEL: define {{.*}} @round_trip(
// LOWERED-HEX-NOT: bitcast
// LOWERED-HEX: store <32 x i64> %v, ptr %agg.result
// LOWERED-HEX: ret void

extern "C" Vec add(Vec v) {
  auto bs = ripple_set_block_shape(0, 32);
  auto x = __builtin_ripple_from_fixed_vector(bs, v);
  return __builtin_ripple_to_fixed_vector(32, x + 1);
}
// LOWERED-HEX-LABEL: define {{.*}} @add(
// LOWERED-HEX: [[SUM:%.*]] = add <32 x i64> %v, splat (i64 1)
// LOWERED-HEX: store <32 x i64> [[SUM]], ptr %agg.result
// LOWERED-HEX: ret void

extern "C" int single_lane(int x) {
  auto bs = ripple_set_block_shape(0, 1);
  return ripple_to_vec<1>(x);
}
// FRONTEND-LABEL: define {{.*}} i32 @single_lane(i32 {{.*}}%x)
// FRONTEND: [[V:%.*]] = call <1 x i32> @llvm.ripple.to.fixed.vector.v1i32
// FRONTEND-NEXT: [[X:%.*]] = bitcast <1 x i32> [[V]] to i32
// FRONTEND-NEXT: ret i32 [[X]]
// LOWERED-HEX-LABEL: define {{.*}} i32 @single_lane(i32 {{.*}}%x)
// LOWERED-HEX: ret i32 %x

extern "C" Vec rank2(Vec v) {
  auto bs = ripple_set_block_shape(0, 8, 4);
  return ripple_to_vec<32>(vec_to_ripple(bs, v));
}
// LOWERED-HEX-LABEL: define {{.*}} @rank2(
// LOWERED-HEX: store <32 x i64> %v, ptr %agg.result
// LOWERED-HEX: ret void

extern "C" Vec rank3(Vec v) {
  auto bs = ripple_set_block_shape(0, 4, 4, 2);
  return ripple_to_vec<32>(vec_to_ripple(bs, v));
}
// LOWERED-HEX-LABEL: define {{.*}} @rank3(
// LOWERED-HEX: store <32 x i64> %v, ptr %agg.result
// LOWERED-HEX: ret void

// LOWERED-GENERIC-LABEL: define {{.*}} @round_trip(
// LOWERED-GENERIC-NOT: bitcast
// LOWERED-GENERIC: {{(ret <32 x i64>|store <32 x i64>)}}
// LOWERED-GENERIC-LABEL: define {{.*}} @add(
// LOWERED-GENERIC: add <32 x i64> {{.*}}, splat (i64 1)
// LOWERED-GENERIC: {{(ret <32 x i64>|store <32 x i64>)}}
// LOWERED-GENERIC-LABEL: define {{.*}} i32 @single_lane(i32 {{.*}}%x)
// LOWERED-GENERIC: ret i32 %x
// LOWERED-GENERIC-LABEL: define {{.*}} @rank2(
// LOWERED-GENERIC: {{(ret <32 x i64>|store <32 x i64>)}}
// LOWERED-GENERIC-LABEL: define {{.*}} @rank3(
// LOWERED-GENERIC: {{(ret <32 x i64>|store <32 x i64>)}}
