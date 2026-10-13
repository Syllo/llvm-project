; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

define void @gather_load_nontemporal(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @gather_load_nontemporal
; CHECK: call <4 x i32> @llvm.masked.gather.v4i32.v4p0({{.*}}), !nontemporal [[NT:![0-9]+]]
; CHECK-NOT: llvm.masked.load
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = shl i32 %v0, 3
  %ptr = getelementptr i32, ptr %A, i32 %idx
  %val = load i32, ptr %ptr, align 4, !nontemporal !0
  %out = getelementptr i32, ptr %B, i32 %v0
  store i32 %val, ptr %out, align 4
  ret void
}

!0 = !{i32 1}
; CHECK: [[NT]] = !{i32 1}
