; REQUIRES: hexagon-registered-target, asserts
; RUN: opt -disable-output -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple)' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes -debug-only=ripple %s 2>&1 | FileCheck %s

; A specialization keeps its <3 x float> ABI, but its body is padded to the
; legal 32-lane HVX width. Its arguments are widened at entry and its result
; is narrowed again at the return boundary.

target triple = "hexagon"

define internal float @mul(float %a, float %b) #1 {
entry:
  %m = fmul float %a, %b
  ret float %m
}

define dso_local void @foo(ptr %A, ptr %B) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr inbounds float, ptr %A, i32 %v
  %a = load float, ptr %pa, align 4
  %pb = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %pb, align 4
  %r = call float @mul(float %a, float %b)
  store float %r, ptr %pa, align 4
  ret void
}

; CHECK-LABEL: define internal fastcc <3 x float> @ripple.specialization.final.1.mul(
; CHECK-SAME: <3 x float> [[ARG0:%[0-9]+]], <3 x float> [[ARG1:%[0-9]+]])
; CHECK-DAG: [[PAD_ARG0:%.*]] = shufflevector <3 x float> [[ARG0]], <3 x float> poison, <32 x i32>
; CHECK-DAG: [[PAD_ARG1:%.*]] = shufflevector <3 x float> [[ARG1]], <3 x float> poison, <32 x i32>
; CHECK: [[PRODUCT:%.*]] = fmul <32 x float> [[PAD_ARG0]], [[PAD_ARG1]]
; CHECK: [[RESULT:%.*]] = call <3 x float> @llvm.vector.extract.v3f32.v32f32(<32 x float> [[PRODUCT]], i64 0)
; CHECK: ret <3 x float> [[RESULT]]

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
