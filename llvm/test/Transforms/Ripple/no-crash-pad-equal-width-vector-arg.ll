; REQUIRES: hexagon-registered-target, asserts
; RUN: opt -disable-output -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple)' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes < %s

; A 16-lane double vector already fills a 128-byte HVX vector. Padding a
; specialization argument to that same width must leave it unchanged.

target triple = "hexagon"

define internal double @mul(double %a, double %b) #1 {
entry:
  %m = fmul double %a, %b
  ret double %m
}

define dso_local void @foo(ptr %A, ptr %B) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 16, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
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

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
