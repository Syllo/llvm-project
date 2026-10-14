; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; If-conversion replaces contiguous vector loads and stores with masked
; intrinsics. The replacements must keep the nontemporal hint.
define void @if_convert_nontemporal(i32 %n, ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @if_convert_nontemporal
; CHECK: [[VAL:%.*]] = call <4 x i32> @llvm.masked.load.v4i32.p0(ptr align 4 %A, <4 x i1> [[ACTIVE:%.*]], <4 x i32> poison), !nontemporal [[NT:![0-9]+]]
; CHECK: call void @llvm.masked.store.v4i32.p0(<4 x i32> [[VAL]], ptr align 4 %B, <4 x i1> [[ACTIVE]]), !nontemporal [[NT]]
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %active = icmp ult i32 %v0, %n
  br i1 %active, label %then, label %exit

then:
  %ptr = getelementptr i32, ptr %A, i32 %v0
  %val = load i32, ptr %ptr, align 4, !nontemporal !0
  %out = getelementptr i32, ptr %B, i32 %v0
  store i32 %val, ptr %out, align 4, !nontemporal !0
  br label %exit

exit:
  ret void
}

!0 = !{i32 1}
; CHECK: [[NT]] = !{i32 1}
