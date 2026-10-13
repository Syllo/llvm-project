; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; The source GEP strides in i32 units while the load is i16. The two
; coalesced windows must therefore be based at byte offsets 0 and 8.
define void @multi_window_source_type_scaling(ptr readonly %A,
                                              ptr writeonly %B) {
; CHECK-LABEL: define void @multi_window_source_type_scaling
; CHECK-NOT: llvm.masked.gather
; CHECK: [[LOAD0:%.*]] = call <4 x i16> @llvm.masked.load.v4i16.p0(ptr align 2 %A, <4 x i1> <i1 true, i1 false, i1 true, i1 false>
; CHECK: [[BASE1:%.*]] = getelementptr i8, ptr %A, i32 8
; CHECK: [[LOAD1:%.*]] = call <4 x i16> @llvm.masked.load.v4i16.p0(ptr align 2 [[BASE1]], <4 x i1> <i1 true, i1 false, i1 true, i1 false>
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %ptr = getelementptr i32, ptr %A, i32 %v0
  %val = load i16, ptr %ptr, align 2
  %out = getelementptr i16, ptr %B, i32 %v0
  store i16 %val, ptr %out, align 2
  ret void
}
