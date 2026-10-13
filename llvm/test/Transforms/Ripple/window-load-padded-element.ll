; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; Scalar i19 objects occupy four bytes, but vector i19 elements are packed.
; Retain a gather rather than treating scalar allocation size as vector stride.
define void @window_load_padded_element(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @window_load_padded_element
; CHECK: call <4 x i19> @llvm.masked.gather.v4i19.v4p0
; CHECK-NOT: llvm.masked.load
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = sub i32 3, %v0
  %ptr = getelementptr i19, ptr %A, i32 %idx
  %val = load i19, ptr %ptr, align 4
  %out = getelementptr i19, ptr %B, i32 %v0
  store i19 %val, ptr %out, align 4
  ret void
}
