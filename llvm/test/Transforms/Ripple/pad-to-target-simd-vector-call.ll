; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple)' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - 2>/dev/null | FileCheck %s

; A padded caller narrows vector values at a call ABI boundary, then widens
; the fixed-width result for further padded uses.

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

; CHECK-LABEL: define dso_local void @foo(
; CHECK: [[CALL_ARG0:%.*]] = call <3 x float> @llvm.vector.extract.v3f32.v32f32(<32 x float> %{{.*}}, i64 0)
; CHECK: [[CALL_ARG1:%.*]] = call <3 x float> @llvm.vector.extract.v3f32.v32f32(<32 x float> %{{.*}}, i64 0)
; CHECK: [[CALL_RESULT:%.*]] = call {{.*}}<3 x float> @ripple.specialization.final.1.mul(<3 x float> [[CALL_ARG0]], <3 x float> [[CALL_ARG1]])
; CHECK: [[PADDED_RESULT:%.*]] = shufflevector <3 x float> [[CALL_RESULT]], <3 x float> poison, <32 x i32>
; CHECK: call void @llvm.masked.store.v32f32.p0(<32 x float> [[PADDED_RESULT]], ptr align 4 %{{.*}}, <32 x i1> {{.*}})

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
