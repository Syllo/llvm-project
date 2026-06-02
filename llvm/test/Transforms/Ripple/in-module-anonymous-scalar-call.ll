; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; Coverage for anonymous-namespace scalar call names.
;
; Existing coverage exercises anonymous-namespace candidate names. This file
; adds the scalar side: Itanium and MSVC anonymous scalar calls should normalize
; to empty namespace and match a global ripple_ candidate with the same base.

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

; ---------------------------------------------------------------------------
; Itanium anonymous-namespace scalar call:
;   _ZN12_GLOBAL__N_16myaddfEff -> (anonymous namespace)::myaddf(float,float)
; ---------------------------------------------------------------------------
define dso_local void @test_itanium_anon_scalar_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_itanium_anon_scalar_call(
; CHECK:         call <32 x float> @ripple_ew_pure_myaddf(
; CHECK-NOT:     call float @_ZN12_GLOBAL__N_16myaddfEff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_ZN12_GLOBAL__N_16myaddfEff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_ZN12_GLOBAL__N_16myaddfEff(float, float)

; ---------------------------------------------------------------------------
; MSVC anonymous-namespace scalar call:
;   ?myaddf@?A0x66F0A327@@YAMMM@Z -> `anonymous namespace'::myaddf(float,float)
; ---------------------------------------------------------------------------
define dso_local void @test_msvc_anon_scalar_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_msvc_anon_scalar_call(
; CHECK:         call <32 x float> @ripple_ew_pure_myaddf(
; CHECK-NOT:     call float @"?myaddf@?A0x66F0A327@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?myaddf@?A0x66F0A327@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?myaddf@?A0x66F0A327@@YAMMM@Z"(float, float)

define internal <32 x float> @ripple_ew_pure_myaddf(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
