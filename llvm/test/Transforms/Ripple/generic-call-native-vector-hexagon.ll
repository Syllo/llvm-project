; REQUIRES: hexagon-registered-target
; RUN: opt -passes='ripple' -verify-each -S %s | FileCheck %s

; Hexagon scalarizes generic calls through argument/result buffers. A native
; vector operand still has scalar Ripple shape and must be passed unchanged to
; each scalar call.
target triple = "hexagon"

define void @generic_native_vector(ptr %in, ptr %out, <2 x i64> %v) {
; CHECK-LABEL: define void @generic_native_vector(
; CHECK: call i64 @scalar_with_native_vector(<2 x i64> %v, i64 %{{.*}})
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %src = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %src, align 8
  %r = call i64 @scalar_with_native_vector(<2 x i64> %v, i64 %x)
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %r, ptr %dst, align 8
  ret void
}

declare i64 @scalar_with_native_vector(<2 x i64>, i64)
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
