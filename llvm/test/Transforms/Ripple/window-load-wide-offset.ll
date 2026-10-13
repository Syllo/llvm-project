; RUN: opt -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; Window byte offsets are materialized in the pointer index type: the i32
; float index 2^30 is the byte offset 2^32.

; Logical lanes access [2^30, 2^30, 2^30 + 1, 2^30 + 1].
define void @single_window_wide_offset(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @single_window_wide_offset
; CHECK-NOT: llvm.masked.gather
; CHECK: getelementptr i8, ptr %A, i64 4294967296
; CHECK: llvm.masked.load
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %half = lshr i32 %v0, 1
  %idx = add i32 %half, 1073741824
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = load float, ptr %ptr, align 4
  %out = getelementptr float, ptr %B, i32 %v0
  store float %val, ptr %out, align 4
  ret void
}

; Logical lanes access [0, 1, 2^30, 2^30 + 1].
define void @multi_window_wide_offset(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @multi_window_wide_offset
; CHECK-NOT: llvm.masked.gather
; CHECK: llvm.masked.load{{.*}}(ptr align 4 %A,
; CHECK: [[WINDOW1:%.*]] = getelementptr i8, ptr %A, i64 4294967296
; CHECK: llvm.masked.load{{.*}}(ptr align 4 [[WINDOW1]],
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %lo = and i32 %v0, 1
  %hi = lshr i32 %v0, 1
  %hi.scaled = shl i32 %hi, 30
  %idx = add i32 %lo, %hi.scaled
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = load float, ptr %ptr, align 4
  %out = getelementptr float, ptr %B, i32 %v0
  store float %val, ptr %out, align 4
  ret void
}
