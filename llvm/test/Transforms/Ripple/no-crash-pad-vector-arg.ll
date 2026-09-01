; REQUIRES: hexagon-registered-target
; RUN: opt -S -passes='ripple' -ripple-pad-to-target-simd %s -o - | FileCheck %s

target triple = "hexagon"

define internal double @mul(double %a, double %b) #1 {
entry:
  %m = fmul double %a, %b
  ret double %m
}

define dso_local void @foo(ptr %A, ptr %B) #0 {
; CHECK-LABEL: define dso_local void @foo(
; CHECK: load <33 x double>
; CHECK: load <33 x double>
; CHECK: call fastcc <33 x double> @ripple.specialization.final.1.mul(
; CHECK: store <33 x double>
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

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-cpu"="hexagonv81" "target-features"="+hvx-length128b,+hvxv81,+v81,-long-calls" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" "target-cpu"="hexagonv81" "target-features"="+hvx-length128b,+hvxv81,+v81,-long-calls" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
