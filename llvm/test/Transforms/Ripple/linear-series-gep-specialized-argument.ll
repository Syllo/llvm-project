; RUN: opt -passes=ripple -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; The caller passes a scalar offset and its block shape to the callee. Ripple
; specializes the callee and must continue the narrow GEP index series through
; the scalar-shaped specialization argument.

; CHECK-LABEL: define void @caller(
; CHECK: call fastcc void @ripple.specialization.final.{{[0-9]+}}.callee
define void @caller(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %trigger = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  call void @callee(ptr %A, i32 %n, ptr %BS, i32 %trigger)
  ret void
}

define internal void @callee(ptr %A, i32 %n, ptr %BS, i32 %trigger) noinline {
entry:
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = add nsw i32 %n, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; A varying specialization argument is not a non-wrapping leaf. The GEP must
; retain its per-lane extension instead of becoming a contiguous pointer series.

; CHECK-LABEL: define void @caller_varying_argument(
; CHECK: call fastcc void @ripple.specialization.final.{{[0-9]+}}.callee_varying
define void @caller_varying_argument(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %n = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  call void @callee_varying(ptr %A, i32 %n, ptr %BS)
  ret void
}

define internal void @callee_varying(ptr %A, i32 %n, ptr %BS) noinline {
entry:
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = add nsw i32 %n, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; CHECK: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.callee_varying(
; CHECK: %[[IDX:.*]] = add <4 x i32> %{{.*}}, <i32 0, i32 1, i32 2, i32 3>
; CHECK: %[[IDX64:.*]] = sext <4 x i32> %[[IDX]] to <4 x i64>
; CHECK: getelementptr i8, ptr %{{.*}}, <4 x i64> %[[IDX64]]
; CHECK: call void @llvm.masked.scatter.v4i8.v4p0(<4 x i8> {{.*}}, <4 x ptr> align 1 {{.*}}, <4 x i1> {{.*}})
; CHECK: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.callee(
; CHECK: %[[N:.*]] = sext i32 %{{.*}} to i64
; CHECK: getelementptr i8, ptr %{{.*}}, i64 %[[N]]
; CHECK: store <4 x i8> {{.*}}, ptr {{.*}}, align 1
