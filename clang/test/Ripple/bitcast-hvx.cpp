// REQUIRES: hexagon-registered-target
// RUN: %clang --target=hexagon -mv79 -mhvx -mhvx-length=128B -fenable-ripple -O2 -S -emit-llvm %s -o - | FileCheck %s --implicit-check-not=alloca --implicit-check-not=llvm.ripple.bitcast --implicit-check-not=llvm.masked.load

#include <hexagon_protos.h>
#include <hexagon_types.h>
#include <ripple.h>

// The HVX pair is represented as <64 x i32>, but the conversion element type
// is explicitly selected as i64. Even inside divergent control flow the
// conversion needs no temporary array: only the final output store is masked.
extern "C" void packed_to_ripple(HVX_Vector hi, HVX_Vector lo,
                                long long *out, unsigned count) {
  auto bs = ripple_set_block_shape(0, 32);
  auto id = ripple_id(bs, 0);
  if (id < count) {
    HVX_VectorPair packed = Q6_W_vshuff_VVR(hi, lo, -4);
    out[id] = vec_to_ripple<long long>(bs, packed);
  }
}
// CHECK-LABEL: define {{.*}} @packed_to_ripple(
// CHECK: [[PACKED:%.*]] = {{.*}}call <64 x i32> @llvm.hexagon.V6.vshuffvdd.128B
// CHECK: [[VIEW:%.*]] = bitcast <64 x i32> [[PACKED]] to <32 x i64>
// CHECK: call void @llvm.masked.store.v32i64.p0(<32 x i64> [[VIEW]]
