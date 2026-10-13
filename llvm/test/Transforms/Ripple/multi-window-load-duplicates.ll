; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; Duplicates in separate windows must preserve their result lanes.
define void @multi_window_duplicates(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @multi_window_duplicates
; CHECK-NOT: llvm.masked.gather
; CHECK: [[LOAD0:%.*]] = call <8 x i16> @llvm.masked.load.v8i16.p0(ptr align 2 %A, <8 x i1> <i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>
; CHECK: shufflevector <8 x i16> {{.*}}<8 x i32> <i32 0, i32 0, i32 1, i32 1,
; CHECK: [[BASE1:%.*]] = getelementptr i8, ptr %A, i32 16
; CHECK: [[LOAD1:%.*]] = call <8 x i16> @llvm.masked.load.v8i16.p0(ptr align 2 [[BASE1]], <8 x i1> <i1 true, i1 true, i1 false, i1 false, i1 false, i1 false, i1 false, i1 false>
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 8, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)

  ; idx: 0, 0, 1, 1, 8, 8, 9, 9
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
  ret void
}
