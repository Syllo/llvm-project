// REQUIRES: hexagon-registered-target
// RUN: %clang --target=hexagon -mv79 -mhvx -mhvx-length=128B -fenable-ripple -O2 -S -emit-llvm %s -o - | FileCheck %s --implicit-check-not=alloca --implicit-check-not=llvm.ripple.bitcast --implicit-check-not=llvm.masked.load

#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <ripple.h>

// The HVX pair is represented as <64 x i32>, but the conversion element type
// is explicitly reinterpreted as i64. Even inside divergent control flow the
// conversion needs no temporary array: only the final output store is masked.
extern "C" void packed_to_ripple(HVX_Vector hi, HVX_Vector lo,
                                long long *out, unsigned count) {
  auto bs = ripple_set_block_shape(0, 32);
  auto id = ripple_id(bs, 0);
  if (id < count) {
    HVX_VectorPair packed = Q6_W_vshuff_VVR(hi, lo, -4);
    using Vec = long long __attribute__((vector_size(sizeof(HVX_VectorPair))));
    out[id] = vec_to_ripple(bs, reinterpret_cast<Vec>(packed));
  }
}
// CHECK-LABEL: define {{.*}} @packed_to_ripple(
// CHECK: [[PACKED:%.*]] = {{.*}}call <64 x i32> @llvm.hexagon.V6.vshuffvdd.128B
// CHECK: [[VIEW:%.*]] = bitcast <64 x i32> [[PACKED]] to <32 x i64>
// CHECK: call void @llvm.masked.store.v32i64.p0(<32 x i64> [[VIEW]]

// The Ripple reinterpretation uses the source vector's 64-lane shape first,
// then the 32-lane destination shape. It must produce the same bitcast/store.
extern "C" void packed_to_ripple_reinterp(HVX_Vector hi, HVX_Vector lo,
                                         long long *out, unsigned count) {
  auto source = ripple_set_block_shape(0, 64);
  auto destination = ripple_set_block_shape(0, 32);
  auto id = ripple_id(destination, 0);
  if (id < count) {
    HVX_VectorPair packed = Q6_W_vshuff_VVR(hi, lo, -4);
    out[id] = ripple_reinterp_i64(destination, vec_to_ripple(source, packed));
  }
}
// CHECK-LABEL: define {{.*}} @packed_to_ripple_reinterp(
// CHECK: [[PACKED_R:%.*]] = {{.*}}call <64 x i32> @llvm.hexagon.V6.vshuffvdd.128B
// CHECK: [[VIEW_R:%.*]] = bitcast <64 x i32> [[PACKED_R]] to <32 x i64>
// CHECK: call void @llvm.masked.store.v32i64.p0(<32 x i64> [[VIEW_R]]
