; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; Logical lanes access [0, 0, 1, 1, 8, 8, 9, 9]. Each physical lane must be
; enabled when either logical lane using its address is active.
define void @masked_multi_window_load_duplicates(i32 %n, ptr readonly %A,
                                                 ptr writeonly %B) {
; CHECK-LABEL: define void @masked_multi_window_load_duplicates
; CHECK-NOT: llvm.masked.gather
; CHECK: [[ACTIVE:%.*]] = icmp ugt <8 x i32>
; CHECK: [[REMAP0:%.*]] = shufflevector <8 x i1> [[ACTIVE]],
; CHECK-SAME: <8 x i1> <{{[^>]+}}>,
; CHECK-SAME: <8 x i32> <i32 0, i32 2, i32 8, i32 8,
; CHECK-SAME: i32 8, i32 8, i32 8, i32 8>
; CHECK: [[REMAP1:%.*]] = shufflevector <8 x i1> [[ACTIVE]],
; CHECK-SAME: <8 x i1> <{{[^>]+}}>,
; CHECK-SAME: <8 x i32> <i32 1, i32 3, i32 8, i32 8,
; CHECK-SAME: i32 8, i32 8, i32 8, i32 8>
; CHECK: [[PHYSICAL0:%.*]] = or <8 x i1> [[REMAP0]], [[REMAP1]]
; CHECK: [[MASK0:%.*]] = and <8 x i1> [[PHYSICAL0]],
; CHECK-SAME: <i1 true, i1 true, i1 false, i1 false,
; CHECK-SAME: i1 false, i1 false, i1 false, i1 false>
; CHECK: [[LOAD0:%.*]] = call <8 x i16> @llvm.masked.load.v8i16.p0(
; CHECK-SAME: ptr align 2 %A, <8 x i1> [[MASK0]], <8 x i16> poison)
; CHECK: [[LOGICAL0:%.*]] = shufflevector <8 x i16> [[LOAD0]],
; CHECK-SAME: <8 x i16> poison,
; CHECK-SAME: <8 x i32> <i32 0, i32 0, i32 1, i32 1,
; CHECK-SAME: i32 poison, i32 poison, i32 poison, i32 poison>
; CHECK: [[WINDOW1:%.*]] = getelementptr i8, ptr %A, i32 16
; CHECK: [[REMAP2:%.*]] = shufflevector <8 x i1> [[ACTIVE]],
; CHECK-SAME: <8 x i1> <{{[^>]+}}>,
; CHECK-SAME: <8 x i32> <i32 4, i32 6, i32 8, i32 8,
; CHECK-SAME: i32 8, i32 8, i32 8, i32 8>
; CHECK: [[REMAP3:%.*]] = shufflevector <8 x i1> [[ACTIVE]],
; CHECK-SAME: <8 x i1> <{{[^>]+}}>,
; CHECK-SAME: <8 x i32> <i32 5, i32 7, i32 8, i32 8,
; CHECK-SAME: i32 8, i32 8, i32 8, i32 8>
; CHECK: [[PHYSICAL1:%.*]] = or <8 x i1> [[REMAP2]], [[REMAP3]]
; CHECK: [[MASK1:%.*]] = and <8 x i1> [[PHYSICAL1]],
; CHECK-SAME: <i1 true, i1 true, i1 false, i1 false,
; CHECK-SAME: i1 false, i1 false, i1 false, i1 false>
; CHECK: [[LOAD1:%.*]] = call <8 x i16> @llvm.masked.load.v8i16.p0(
; CHECK-SAME: ptr align 2 [[WINDOW1]], <8 x i1> [[MASK1]], <8 x i16> poison)
; CHECK: [[LOGICAL1:%.*]] = shufflevector <8 x i16> [[LOAD1]],
; CHECK-SAME: <8 x i16> poison,
; CHECK-SAME: <8 x i32> <i32 poison, i32 poison, i32 poison, i32 poison,
; CHECK-SAME: i32 0, i32 0, i32 1, i32 1>
; CHECK: [[RESULT:%.*]] = shufflevector <8 x i16> [[LOGICAL0]],
; CHECK-SAME: <8 x i16> [[LOGICAL1]],
; CHECK-SAME: <8 x i32> <i32 0, i32 1, i32 2, i32 3,
; CHECK-SAME: i32 12, i32 13, i32 14, i32 15>
; CHECK: call void @llvm.masked.store.v8i16.p0(<8 x i16> [[RESULT]],
; CHECK-SAME: ptr align 2 %B, <8 x i1> [[ACTIVE]])
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 8, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %active = icmp ult i32 %v0, %n
  br i1 %active, label %then, label %exit

then:
  %is2 = icmp ult i32 %v0, 2
  %is4 = icmp ult i32 %v0, 4
  %is6 = icmp ult i32 %v0, 6
  %lo = select i1 %is2, i32 0, i32 1
  %hi = select i1 %is6, i32 8, i32 9
  %idx = select i1 %is4, i32 %lo, i32 %hi
  %ptr = getelementptr i16, ptr %A, i32 %idx
  %val = load i16, ptr %ptr, align 2
  %out = getelementptr i16, ptr %B, i32 %v0
  store i16 %val, ptr %out, align 2
  br label %exit

exit:
  ret void
}
