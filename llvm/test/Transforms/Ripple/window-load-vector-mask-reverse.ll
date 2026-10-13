; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)

; Reverse logical lanes must reverse the branch mask before applying it to the
; physical load lanes.
define void @masked_window_load_reverse(i32 %n, ptr readonly %A,
                                        ptr writeonly %B) {
; CHECK-LABEL: define void @masked_window_load_reverse
; CHECK-NOT: llvm.masked.gather
; CHECK: [[ACTIVE:%.*]] = icmp ugt <4 x i32>
; CHECK: [[PHYSICAL:%.*]] = shufflevector <4 x i1> [[ACTIVE]],
; CHECK-SAME: <4 x i1> poison,
; CHECK-SAME: <4 x i32> <i32 3, i32 2, i32 1, i32 0>
; CHECK: [[LOAD:%.*]] = call <4 x float> @llvm.masked.load.v4f32.p0(
; CHECK-SAME: ptr align 4 %A, <4 x i1> [[PHYSICAL]], <4 x float> poison)
; CHECK: [[RESULT:%.*]] = shufflevector <4 x float> [[LOAD]],
; CHECK-SAME: <4 x float> poison,
; CHECK-SAME: <4 x i32> <i32 3, i32 2, i32 1, i32 0>
; CHECK: call void @llvm.masked.store.v4f32.p0(<4 x float> [[RESULT]],
; CHECK-SAME: ptr align 4 %B, <4 x i1> [[ACTIVE]])
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %active = icmp ult i32 %v0, %n
  br i1 %active, label %then, label %exit

then:
  %idx = sub i32 3, %v0
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = load float, ptr %ptr, align 4
  %out = getelementptr float, ptr %B, i32 %v0
  store float %val, ptr %out, align 4
  br label %exit

exit:
  ret void
}
