; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; Negative test: @ripple_ew_pure_myaddf does NOT carry the
; "ripple-vector-candidate" attribute, so registerInModuleRippleCandidates()
; skips it.  No -ripple-lib is passed either.  The ripple pass must NOT replace
; the scalar @myaddf call with the vector candidate.

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

define dso_local void @foo(ptr %A, ptr %B, ptr %C) #0 {
; @ripple_ew_pure_myaddf must not appear: no attribute means not registered.
; CHECK-LABEL: define dso_local void @foo(
; CHECK-NOT: call <32 x float> @ripple_ew_pure_myaddf
; CHECK: call float @myaddf(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %arrayidx = getelementptr inbounds float, ptr %B, i32 %v
  %bvec = load float, ptr %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds float, ptr %C, i32 %v
  %cvec = load float, ptr %arrayidx1, align 4
  %call = call float @myaddf(float %bvec, float %cvec)
  %arrayidx2 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %arrayidx2, align 4
  ret void
}

; Scalar function called inside the ripple block.
declare float @myaddf(float, float)

; Candidate declared but WITHOUT the "ripple-vector-candidate" attribute:
; registerInModuleRippleCandidates() must skip it.
declare <32 x float> @ripple_ew_pure_myaddf(<32 x float>, <32 x float>)

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
