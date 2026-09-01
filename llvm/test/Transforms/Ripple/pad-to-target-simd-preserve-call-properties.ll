; REQUIRES: x86-registered-target
; RUN: opt -S -passes='module(ripple)' -ripple-pad-to-target-simd %s -o - 2>/dev/null | FileCheck %s

; Padding recreates this vector call after the specialization's vector return
; causes padding to bail out.  The replacement call must retain fastcc: the
; specialization definition is fastcc, and a default-C call is undefined.

target triple = "i386-unknown-linux-gnu"

define internal double @mul(double %a, double %b) #1 {
entry:
  %m = fmul double %a, %b
  ret double %m
}

define dso_local void @foo(ptr %A, ptr %B) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 33, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr inbounds double, ptr %A, i32 %v
  %a = load double, ptr %pa, align 8
  %pb = getelementptr inbounds double, ptr %B, i32 %v
  %b = load double, ptr %pb, align 8
  %r = call double @mul(double %a, double %b)
  store double %r, ptr %pa, align 8
  ret void
}

; CHECK-LABEL: define dso_local void @foo(
; CHECK: call fastcc <33 x double> @ripple.specialization.final.1.mul

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
