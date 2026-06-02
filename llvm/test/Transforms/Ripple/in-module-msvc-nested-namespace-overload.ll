; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; Coverage for two C++ naming gaps on the MSVC ABI:
;   1) nested namespace matching (a::b::myaddf)
;   2) overload discrimination when namespace+base are equal
;      (a::b::myaddf(float,float) vs a::b::myaddf(double,double)).
;
; The candidate is a::b::ripple_ew_pure_myaddf with float scalar semantics.
; A float call must match; a double overload call must not match.

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

; ---------------------------------------------------------------------------
; Positive: nested-namespace scalar call a::b::myaddf(float,float).
; ---------------------------------------------------------------------------
define dso_local void @test_nested_namespace_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_nested_namespace_match(
; CHECK:         call <32 x float> @"?ripple_ew_pure_myaddf@b@a@@YA?AT__m1024@@T3@0@Z"(
; CHECK-NOT:     call float @"?myaddf@b@a@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?myaddf@b@a@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?myaddf@b@a@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; Negative: same namespace+base but different overload
; a::b::myaddf(double,double) must not match the float candidate.
; ---------------------------------------------------------------------------
define dso_local void @test_overload_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_overload_no_match(
; CHECK:         call double @"?myaddf@b@a@@YANNN@Z"(
; CHECK-NOT:     call <32 x float> @"?ripple_ew_pure_myaddf@b@a@@YA?AT__m1024@@T3@0@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 16, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds double, ptr %B, i32 %v
  %b = load double, ptr %gep, align 8
  %gep2 = getelementptr inbounds double, ptr %C, i32 %v
  %c = load double, ptr %gep2, align 8
  %call = call double @"?myaddf@b@a@@YANNN@Z"(double %b, double %c)
  %gep3 = getelementptr inbounds double, ptr %A, i32 %v
  store double %call, ptr %gep3, align 8
  ret void
}
declare double @"?myaddf@b@a@@YANNN@Z"(double, double)

; Candidate in namespace a::b (source base name myaddf).
define internal <32 x float> @"?ripple_ew_pure_myaddf@b@a@@YA?AT__m1024@@T3@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
