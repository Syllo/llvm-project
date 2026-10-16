; RUN: opt -passes=ripple -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; The nsw flag on %b only holds for the values of %a, which wraps in i32 for
; lanes 2 and 3 without nsw. These lanes are negative once sign-extended by the
; GEP, so the access is not contiguous.

; CHECK-LABEL: define void @narrow_index_wraps_upstream(
; CHECK: %[[PTRS:.*]] = getelementptr i8, ptr %A, <4 x i64> <i64 2147483646, i64 2147483647, i64 -2147483648, i64 -2147483647>
; CHECK: call void @llvm.masked.scatter.v4i8.v4p0(<4 x i8> {{.*}}, <4 x ptr> align 1 %[[PTRS]],
; CHECK-NOT: store <4 x i8>
define void @narrow_index_wraps_upstream(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %a = add i32 %v0, 2147483646
  %b = add nsw i32 %a, 0
  %ptr = getelementptr i8, ptr %A, i32 %b
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; %m may wrap, but by the same amount in every lane: it has zero slopes, and
; the nsw add of the block index keeps the access contiguous.

; CHECK-LABEL: define void @narrow_index_uniform_wrap_upstream(
; CHECK: %[[M:.*]] = add i32 %n, 5
; CHECK: %[[M64:.*]] = sext i32 %[[M]] to i64
; CHECK: %[[PTR:.*]] = getelementptr [4 x i8], ptr %A, i64 %[[M64]]
; CHECK: store <4 x float> {{.*}}, ptr %[[PTR]], align 4
define void @narrow_index_uniform_wrap_upstream(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %m = add i32 %n, 5
  %idx = add nsw i32 %m, %v0
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = sitofp i32 %v0 to float
  store float %val, ptr %ptr, align 4
  ret void
}
