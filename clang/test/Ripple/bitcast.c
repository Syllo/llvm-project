// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=HEX --implicit-check-not=alloca --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=GENERIC --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -O2 -emit-llvm %s -o - | FileCheck %s --check-prefix=GENERIC --implicit-check-not=llvm.ripple.bitcast %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple hexagon -fenable-ripple -DTEST_TOO_MANY -fsyntax-only -verify -verify-ignore-unexpected=note %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple x86_64-unknown-linux-gnu -fenable-ripple -DTEST_TOO_MANY -fsyntax-only -verify -verify-ignore-unexpected=note %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -I %S/../../lib/Headers/ripple_include -triple aarch64-unknown-linux-gnu -fenable-ripple -DTEST_TOO_MANY -fsyntax-only -verify -verify-ignore-unexpected=note %s %}

#include <ripple.h>
typedef short Vec __attribute__((vector_size(16)));
typedef int RawVec __attribute__((vector_size(16)));

#ifdef TEST_TOO_MANY
void too_many_arguments(int x) {
  ripple_to_vec(4, int, x); // expected-error {{too many arguments provided to function-like macro invocation}}
  // expected-error@-1 {{use of undeclared identifier 'ripple_to_vec'}}
  vec_to_ripple(int, 0, x); // expected-error {{too many arguments provided to function-like macro invocation}}
  // expected-error@-1 {{use of undeclared identifier 'vec_to_ripple'}}
}
#endif

Vec round_trip(Vec v) {
  ripple_block_t bs = ripple_set_block_shape(0, 8);
  return ripple_to_vec(8, vec_to_ripple(bs, v));
}
// HEX-LABEL: define {{.*}} @round_trip(
// HEX: store <8 x i16> %v, ptr %agg.result
// HEX: ret void

Vec explicit_element_type(RawVec v) {
  ripple_block_t bs = ripple_set_block_shape(0, 8);
  return ripple_to_vec(8, vec_to_ripple(bs, (Vec)v));
}
// HEX-LABEL: define {{.*}} @explicit_element_type(
// HEX: store <4 x i32> %v, ptr %agg.result
// HEX: ret void

// Reinterpreting in Ripple must lower identically to the native cast above.
Vec ripple_element_type(RawVec v) {
  ripple_block_t source = ripple_set_block_shape(0, 4);
  ripple_block_t destination = ripple_set_block_shape(0, 8);
  return ripple_to_vec(
      8, ripple_reinterp_i16(destination, vec_to_ripple(source, v)));
}
// HEX-LABEL: define {{.*}} @ripple_element_type(
// HEX: store <4 x i32> %v, ptr %agg.result
// HEX: ret void

int single_lane(int x) {
  ripple_block_t bs = ripple_set_block_shape(0, 1);
  return ripple_to_vec(1, x);
}
// HEX-LABEL: define {{.*}} i32 @single_lane(i32 {{.*}}%x)
// HEX: ret i32 %x

Vec rank2(Vec v) {
  ripple_block_t bs = ripple_set_block_shape(0, 4, 2);
  return ripple_to_vec(8, vec_to_ripple(bs, v));
}
// HEX-LABEL: define {{.*}} @rank2(
// HEX: store <8 x i16> %v, ptr %agg.result
// HEX: ret void

Vec rank3(Vec v) {
  ripple_block_t bs = ripple_set_block_shape(0, 2, 2, 2);
  return ripple_to_vec(8, vec_to_ripple(bs, v));
}
// HEX-LABEL: define {{.*}} @rank3(
// HEX: store <8 x i16> %v, ptr %agg.result
// HEX: ret void

// GENERIC-LABEL: define {{.*}} @round_trip(
// GENERIC: ret <8 x i16> %{{.*}}
// GENERIC-LABEL: define {{.*}} @explicit_element_type(
// GENERIC: bitcast <4 x i32> %{{.*}} to <8 x i16>
// GENERIC: ret <8 x i16> %{{.*}}
// GENERIC-LABEL: define {{.*}} @ripple_element_type(
// GENERIC: bitcast <4 x i32> %{{.*}} to <8 x i16>
// GENERIC: ret <8 x i16> %{{.*}}
// GENERIC-LABEL: define {{.*}} i32 @single_lane(i32 {{.*}}%x)
// GENERIC: ret i32 %x
// GENERIC-LABEL: define {{.*}} @rank2(
// GENERIC: ret <8 x i16> %{{.*}}
// GENERIC-LABEL: define {{.*}} @rank3(
// GENERIC: ret <8 x i16> %{{.*}}
