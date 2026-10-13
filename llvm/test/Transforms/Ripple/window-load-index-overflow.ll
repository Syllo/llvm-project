; RUN: opt -mtriple=i386-unknown-linux-gnu -passes=ripple,dce -S %s | FileCheck %s --implicit-check-not="warning:"

target datalayout = "e-p:32:32-i64:64-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i64(i64, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64)

; Scaling the two large i64 indices by the i64 GEP source size overflows.
; Reject the window optimization and retain the byte-accurate gather.
define void @window_load_index_overflow(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @window_load_index_overflow
; CHECK: call <4 x i16> @llvm.masked.gather.v4i16.v4p0
; CHECK-NOT: llvm.masked.load
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v0 = call i64 @llvm.ripple.block.index.i64(ptr %BS, i64 0)
  %is0 = icmp eq i64 %v0, 0
  %is1 = icmp eq i64 %v0, 1
  %is2 = icmp eq i64 %v0, 2
  %idx2 = select i1 %is2, i64 9223372036854775807,
                         i64 9223372036854775806
  %idx1 = select i1 %is1, i64 1, i64 %idx2
  %idx = select i1 %is0, i64 0, i64 %idx1
  %ptr = getelementptr i64, ptr %A, i64 %idx
  %val = load i16, ptr %ptr, align 2
  %out = getelementptr i16, ptr %B, i64 %v0
  store i16 %val, ptr %out, align 2
  ret void
}
