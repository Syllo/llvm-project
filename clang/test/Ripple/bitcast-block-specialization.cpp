// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=HEX --implicit-check-not=alloca --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=X86 --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=AARCH64 --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -Rpass=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -Rpass=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -Rpass=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK %}

// Keep the conversion helpers out of line. A block specializes the native to
// Ripple conversion, while the Ripple value's shape specializes the reverse
// conversion. Also exercise scalar fallback, including Hexagon's sret ABI.
#include <ripple.h>
using Vec = long long __attribute__((vector_size(256)));
__attribute__((noinline)) static long long from_native(ripple_block_t bs, Vec v) {
  return vec_to_ripple(bs, v);
}
__attribute__((noinline)) static Vec to_native(long long x) {
  return ripple_to_vec<32>(x);
}
extern "C" Vec round_trip(Vec v) {
  auto bs = ripple_set_block_shape(0, 32);
  auto x = from_native(bs, v);
  return to_native(x);
}
extern "C" Vec uniform(long long x) {
  auto bs = ripple_set_block_shape(0, 32);
  return to_native(x);
}

// HEX-LABEL: define {{.*}} @round_trip(
// HEX: [[R:%.*]] = {{.*}}call fastcc <32 x i64> @[[FROM:ripple.specialization.final.[0-9]+.[^(]+]](ptr poison, <32 x i64> %v)
// HEX: call fastcc void @[[TO:ripple.specialization.final.[0-9]+.[^(]+]](ptr {{.*}}%agg.result, <32 x i64> [[R]])
// HEX: define internal fastcc void @[[SPLAT:[^(]+]](
// HEX: [[INSERT:%.*]] = insertelement <32 x i64> poison, i64 %x,
// HEX: [[BROADCAST:%.*]] = shufflevector <32 x i64> [[INSERT]], <32 x i64> poison, <32 x i32> zeroinitializer
// HEX: store <32 x i64> [[BROADCAST]], ptr %agg.result
// HEX-LABEL: define {{.*}} @uniform(
// HEX: call fastcc void @[[SPLAT]](ptr {{.*}}%agg.result, i64 {{.*}}%x)
// HEX: define internal fastcc <32 x i64> @[[FROM]](ptr %0, <32 x i64> %1)
// HEX: ret <32 x i64> %1
// HEX: define internal fastcc void @[[TO]](ptr %0, <32 x i64> %1)
// HEX: store <32 x i64> %1, ptr %0

// X86-LABEL: define {{.*}} @round_trip(
// X86: call fastcc <32 x i64> @ripple.specialization.final
// X86-LABEL: define internal fastcc {{.*}} @_ZL9to_nativex(
// X86: shufflevector <32 x i64>
// X86-LABEL: define {{.*}} @uniform(
// X86: call fastcc {{.*}} @_ZL9to_nativex(

// AARCH64-LABEL: define {{.*}} @round_trip(
// AARCH64: call fastcc <32 x i64> @ripple.specialization.final
// AARCH64-LABEL: define internal fastcc {{.*}} @_ZL9to_nativex(
// AARCH64: shufflevector <32 x i64>
// AARCH64-LABEL: define {{.*}} @uniform(
// AARCH64: call fastcc {{.*}} @_ZL9to_nativex(

// REMARK: remark: call to ripple API 'llvm.ripple.from.fixed.vector.v32i64'
// REMARK: remark: specialized call to {{.*}}from_native
