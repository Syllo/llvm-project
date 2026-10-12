// RUN: %clang -O2 -fenable-ripple -S -emit-llvm -mllvm -print-before-all -mllvm -filter-print-funcs=ripple_i16_index_load_store %s -o /dev/null 2>&1 | FileCheck %s --implicit-check-not="warning:" --implicit-check-not="error:"

#include "ripple_test.h"

void ripple_i16_index_load_store(int *__restrict dst,
                                 const int *__restrict src) {
  ripple_block_t BS = ripple_set_block_shape(0, 128);
  int16_t lane16 = (int16_t)ripple_id(BS, 0);
  dst[lane16] = src[lane16] + 1;
}

// CHECK-LABEL: ; *** IR Dump Before RippleAPIPreprocessPass on ripple_i16_index_load_store ***
// CHECK-LABEL: define{{.*}}void @ripple_i16_index_load_store(
// CHECK: [[ID:%.*]] = call [[SIZE:i(32|64)]] @llvm.ripple.block.index.[[SIZE]](
// CHECK-NEXT: [[TRUNC:%.*]] = trunc [[SIZE]] [[ID]] to i16
// CHECK: [[SEXT:%.*]] = sext i16 {{.*}} to [[SIZE]]
// CHECK: getelementptr inbounds i32, ptr {{.*}}, [[SIZE]] [[SEXT]]
// CHECK: [[SEXT2:%.*]] = sext i16 {{.*}} to [[SIZE]]
// CHECK: getelementptr inbounds i32, ptr {{.*}}, [[SIZE]] [[SEXT2]]

// CHECK-LABEL: ; *** IR Dump Before RippleModulePass on [module] ***
// CHECK-LABEL: define{{.*}}void @ripple_i16_index_load_store(
// CHECK: [[BS:%.*]] = {{.*}}call ptr @llvm.ripple.block.setshape.[[SIZE]]
// CHECK-NEXT: [[ID:%.*]] = {{.*}}call range([[SIZE]] 0, 128) [[SIZE]] @llvm.ripple.block.index.[[SIZE]](ptr [[BS]], [[SIZE]] 0), !range
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: getelementptr{{.*}}, ptr %src, [[SIZE]] [[ID]]
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: getelementptr{{.*}}, ptr %dst, [[SIZE]] [[ID]]
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: ret void
