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
// CHECK: [[ID:%.*]] = call i32 @llvm.ripple.block.index.i32(
// CHECK-NEXT: [[TRUNC:%.*]] = trunc i32 [[ID]] to i16
// CHECK: [[SEXT:%.*]] = sext i16 {{.*}} to i32
// CHECK: getelementptr inbounds i32, ptr {{.*}}, i32 [[SEXT]]
// CHECK: [[SEXT2:%.*]] = sext i16 {{.*}} to i32
// CHECK: getelementptr inbounds i32, ptr {{.*}}, i32 [[SEXT2]]

// CHECK-LABEL: ; *** IR Dump Before RippleModulePass on [module] ***
// CHECK-LABEL: define{{.*}}void @ripple_i16_index_load_store(
// CHECK: [[BS:%.*]] = {{.*}}call ptr @llvm.ripple.block.setshape.i32
// CHECK-NEXT: [[ID:%.*]] = {{.*}}call range(i32 0, 128) i32 @llvm.ripple.block.index.i32(ptr [[BS]], i32 0), !range
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: getelementptr{{.*}}, ptr %src, i32 [[ID]]
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: getelementptr{{.*}}, ptr %dst, i32 [[ID]]
// CHECK-NOT: trunc
// CHECK-NOT: sext
// CHECK-NOT: = and
// CHECK: ret void
