; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; Test that the Ripple pass handles in-module candidates whose LLVM name is a
; C++ mangled symbol — covering all three cases:
;   1. C linkage (no mangling)          — fast path, name starts with ripple_
;   2. Itanium internal (_ZL prefix)    — static inline in C++ TU
;   3. MSVC mangling (? prefix)         — static inline under /MT or /MD
;
; In each case the pass must demangle the name, recognise the ripple_ prefix,
; extract the scalar base name, and replace the scalar call inside @foo with
; the vector candidate.

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

; ---------------------------------------------------------------------------
; Case 1: C linkage — name is already ripple_ew_pure_c_fn.
; ---------------------------------------------------------------------------
define dso_local void @test_c_linkage(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_c_linkage(
; CHECK:         call <32 x float> @ripple_ew_pure_c_fn(
; CHECK-NOT:     call float @c_fn(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @c_fn(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @c_fn(float, float)
define internal <32 x float> @ripple_ew_pure_c_fn(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; ---------------------------------------------------------------------------
; Case 2: Itanium internal (_ZL) — static inline in a C++ TU.
;   _ZL21ripple_ew_pure_itanfn decodes to ripple_ew_pure_itanfn.
;   Scalar base name (after stripping ripple_ + options): itanfn.
; ---------------------------------------------------------------------------
define dso_local void @test_itanium_internal(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_itanium_internal(
; CHECK:         call <32 x float> @_ZL21ripple_ew_pure_itanfnDv32_fS_(
; CHECK-NOT:     call float @itanfn(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @itanfn(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @itanfn(float, float)
; Mangled name of: static inline f32x32 ripple_ew_pure_itanfn(f32x32, f32x32)
define internal <32 x float> @_ZL21ripple_ew_pure_itanfnDv32_fS_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; ---------------------------------------------------------------------------
; Case 3: MSVC ABI (? prefix).
;   ?ripple_ew_pure_msvccfn@@YA... decodes to ripple_ew_pure_msvccfn.
;   Scalar base name: msvccfn.
;   Note: using a simple scalar (float) prototype to keep the mangling short.
; ---------------------------------------------------------------------------
define dso_local void @test_msvc(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_msvc(
; CHECK:         call <32 x float> @"?ripple_ew_pure_msvccfn@@YA?AT__m1024@@T1@0@Z"(
; CHECK-NOT:     call float @msvccfn(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @msvccfn(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @msvccfn(float, float)
; Mangled name of: static inline f32x32 ripple_ew_pure_msvccfn(f32x32, f32x32) [MSVC]
define internal <32 x float> @"?ripple_ew_pure_msvccfn@@YA?AT__m1024@@T1@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; ---------------------------------------------------------------------------
; Case 4: anonymous-namespace candidate (Itanium _GLOBAL__N_).
;   _ZN12_GLOBAL__N_121ripple_ew_pure_anonfnEDv32_fS0_ decodes to
;   (anonymous namespace)::ripple_ew_pure_anonfn(...).  An anonymous namespace
;   is treated as global, so the candidate (base anonfn) matches a GLOBAL scalar
;   call to anonfn.
; ---------------------------------------------------------------------------
define dso_local void @test_anon_namespace(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_anon_namespace(
; CHECK:         call <32 x float> @_ZN12_GLOBAL__N_121ripple_ew_pure_anonfnEDv32_fS0_(
; CHECK-NOT:     call float @anonfn(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @anonfn(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @anonfn(float, float)
; Mangled name of: f32x32 ripple_ew_pure_anonfn(f32x32, f32x32) in an anonymous namespace.
define internal <32 x float> @_ZN12_GLOBAL__N_121ripple_ew_pure_anonfnEDv32_fS0_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; ---------------------------------------------------------------------------
; Case 5: MSVC anonymous-namespace candidate.
;   ?ripple_ew_pure_msvcanon@?A0x1FD5F39C@@.. demangles to
;     `anonymous namespace'::ripple_ew_pure_msvcanon(...)
;   (MSVC uses backticks instead of Itanium parens for the anonymous
;   namespace).  Like the Itanium case, this must be treated as global so
;   the candidate (base msvcanon) matches a GLOBAL scalar call to msvcanon.
; ---------------------------------------------------------------------------
define dso_local void @test_msvc_anon_namespace(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_msvc_anon_namespace(
; CHECK:         call <32 x float> @"?ripple_ew_pure_msvcanon@?A0x1FD5F39C@@YAT?$__vector@M$0CA@@__clang@@T12@0@Z"(
; CHECK-NOT:     call float @msvcanon(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @msvcanon(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @msvcanon(float, float)
; MSVC-mangled candidate in an anonymous namespace (`?A0x...@`).
define internal <32 x float> @"?ripple_ew_pure_msvcanon@?A0x1FD5F39C@@YAT?$__vector@M$0CA@@__clang@@T12@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
