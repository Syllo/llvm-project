; RUN: opt -passes=ripple -verify-each -S %s | FileCheck %s --implicit-check-not='@ripple.specialization.{{[0-9]+}}.'

; A varying scalar argument triggers specialization without a block argument.
; The temporary scalar prototype must not retain a vector-returning body when
; the function pass verifier runs.
define void @caller(ptr %in, ptr %out) {
; CHECK-LABEL: define void @caller(
; CHECK: call fastcc <4 x double> @[[SQUARE:ripple.specialization.final.[0-9]+.square]](<4 x double>
; CHECK: store <4 x double>
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %src = getelementptr double, ptr %in, i64 %id
  %x = load double, ptr %src
  %r = call double @square(double %x)
  %dst = getelementptr double, ptr %out, i64 %id
  store double %r, ptr %dst
  ret void
}

define internal double @square(double %x) noinline {
  %r = fmul double %x, %x
  ret double %r
}

; CHECK: define internal fastcc <4 x double> @[[SQUARE]](<4 x double>
; CHECK: fmul <4 x double>
; CHECK: ret <4 x double>

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
