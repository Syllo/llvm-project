; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; A negative minimum offset must be represented as a signed i32 byte GEP.
define void @window_negative_indices_i32(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @window_negative_indices_i32
; CHECK-NOT: llvm.masked.gather
; CHECK: getelementptr i8, ptr %A, i32 -8
; CHECK: @llvm.masked.load
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)

  ; idx: -2, -1, 0, -1
  %is0 = icmp eq i32 %v0, 0
  %is1 = icmp eq i32 %v0, 1
  %is2 = icmp eq i32 %v0, 2
  %idx2 = select i1 %is2, i32 0, i32 -1
  %idx1 = select i1 %is1, i32 -1, i32 %idx2
  %idx = select i1 %is0, i32 -2, i32 %idx1

  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = load float, ptr %ptr, align 4
  %out = getelementptr float, ptr %B, i32 %v0
  store float %val, ptr %out, align 4
  ret void
}
