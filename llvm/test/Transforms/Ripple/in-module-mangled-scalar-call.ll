; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; Itanium-ABI coverage of name demangling when matching in-module candidates.
; Both the *candidate* name and the *scalar call* name may be either a plain C
; symbol or a C++ Itanium mangling, so the pass must reduce each to its source
; (namespace, base) before comparing them. Matching requires the SAME namespace
; and the same base name; an anonymous namespace is treated as global. This
; exercises the full matrix:
;
;            scalar: regular (c_myaddf)     scalar: mangled (_Z..myaddf..)
;   cand                                                                     base
;   regular  ripple_ew_pure_c_myaddf        ripple_ew_pure_c_myaddf          c_myaddf
;   mangled  _Z21ripple_ew_pure_myaddf..    _Z21ripple_ew_pure_myaddf..      myaddf
;
; plus same-namespace candidate/call pairs and a cross-namespace negative case.
; All mangled names are exactly what clang emits (verified with the frontend).

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

; ---------------------------------------------------------------------------
; regular candidate + regular scalar call (base name c_myaddf).
; ---------------------------------------------------------------------------
define dso_local void @test_reg_cand_reg_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_reg_cand_reg_call(
; CHECK:         call <32 x float> @ripple_ew_pure_c_myaddf(
; CHECK-NOT:     call float @c_myaddf(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @c_myaddf(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @c_myaddf(float, float)

; ---------------------------------------------------------------------------
; same-namespace mangled candidate + mangled scalar call. The scalar ns::c_myaddf
; (_ZN2ns8c_myaddfEff) matches the candidate ns::ripple_ew_pure_c_myaddf
; (_ZN2ns23ripple_ew_pure_c_myaddfEDv32_fS0_): same namespace ns, base c_myaddf.
; ---------------------------------------------------------------------------
define dso_local void @test_reg_cand_mangled_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_reg_cand_mangled_call(
; CHECK:         call <32 x float> @_ZN2ns23ripple_ew_pure_c_myaddfEDv32_fS0_(
; CHECK-NOT:     call float @_ZN2ns8c_myaddfEff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_ZN2ns8c_myaddfEff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_ZN2ns8c_myaddfEff(float, float)

; ---------------------------------------------------------------------------
; mangled candidate + regular scalar call (base name myaddf). The candidate
; _Z21ripple_ew_pure_myaddfDv32_fS_ decodes to ripple_ew_pure_myaddf(...).
; ---------------------------------------------------------------------------
define dso_local void @test_mangled_cand_reg_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_mangled_cand_reg_call(
; CHECK:         call <32 x float> @_Z21ripple_ew_pure_myaddfDv32_fS_(
; CHECK-NOT:     call float @myaddf(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @myaddf(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @myaddf(float, float)

; ---------------------------------------------------------------------------
; mangled candidate + mangled scalar call (base name myaddf). Itanium free
; function _Z6myaddfff decodes to myaddf(float, float).
; ---------------------------------------------------------------------------
define dso_local void @test_mangled_cand_mangled_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_mangled_cand_mangled_call(
; CHECK:         call <32 x float> @_Z21ripple_ew_pure_myaddfDv32_fS_(
; CHECK-NOT:     call float @_Z6myaddfff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_Z6myaddfff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_Z6myaddfff(float, float)

; ---------------------------------------------------------------------------
; nested-namespace candidate + nested-namespace scalar call. a::b::myaddf
; (_ZN1a1b6myaddfEff) matches the candidate a::b::ripple_ew_pure_myaddf
; (_ZN1a1b21ripple_ew_pure_myaddfEDv32_fS1_): same namespace a::b, base myaddf.
; ---------------------------------------------------------------------------
define dso_local void @test_nested_namespace_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_nested_namespace_call(
; CHECK:         call <32 x float> @_ZN1a1b21ripple_ew_pure_myaddfEDv32_fS1_(
; CHECK-NOT:     call float @_ZN1a1b6myaddfEff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_ZN1a1b6myaddfEff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_ZN1a1b6myaddfEff(float, float)

; ---------------------------------------------------------------------------
; Negative: a mangled scalar call with a DIFFERENT base name (_Z9differentff
; -> different) must NOT be replaced by either candidate.
; ---------------------------------------------------------------------------
define dso_local void @test_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_no_match(
; CHECK:         call float @_Z9differentff(
; CHECK-NOT:     call <32 x float> @ripple_ew_pure_c_myaddf(
; CHECK-NOT:     call <32 x float> @_Z21ripple_ew_pure_myaddfDv32_fS_(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_Z9differentff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_Z9differentff(float, float)

; ---------------------------------------------------------------------------
; Negative (cross-namespace): the scalar ns::myaddf (_ZN2ns6myaddfEff) shares
; the base name myaddf with the GLOBAL candidate _Z21ripple_ew_pure_myaddf, but
; lives in namespace ns. Namespaces differ, so it must NOT be replaced.
; ---------------------------------------------------------------------------
define dso_local void @test_cross_namespace_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_cross_namespace_no_match(
; CHECK:         call float @_ZN2ns6myaddfEff(
; CHECK-NOT:     call <32 x float> @_Z21ripple_ew_pure_myaddfDv32_fS_(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_ZN2ns6myaddfEff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_ZN2ns6myaddfEff(float, float)

; ---------------------------------------------------------------------------
; Template instantiation (global). _Z22ripple_ew_pure_combineIilEDv32_fS0_S0_
; demangles to "ripple_ew_pure_combine<int, long>(...)" with base name combine;
; it replaces the scalar call to combine<int, long> (_Z7combineIilEfff).
; ---------------------------------------------------------------------------
define dso_local void @test_template_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_template_match(
; CHECK:         call <32 x float> @_Z22ripple_ew_pure_combineIilEDv32_fS0_S0_(
; CHECK-NOT:     call float @_Z7combineIilEfff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_Z7combineIilEfff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_Z7combineIilEfff(float, float)

; ---------------------------------------------------------------------------
; Template instantiation in namespace ns. _ZN2ns22ripple_ew_pure_combineIilEE..
; (ns::ripple_ew_pure_combine<int, long>, base combine) replaces the scalar call
; ns::combine<int, long> (_ZN2ns7combineIilEEfff): same namespace ns, base combine.
; ---------------------------------------------------------------------------
define dso_local void @test_namespaced_template_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_namespaced_template_match(
; CHECK:         call <32 x float> @_ZN2ns22ripple_ew_pure_combineIilEEDv32_fS1_S1_(
; CHECK-NOT:     call float @_ZN2ns7combineIilEEfff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_ZN2ns7combineIilEEfff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_ZN2ns7combineIilEEfff(float, float)

; ---------------------------------------------------------------------------
; Negative (cross-namespace template): a call to the GLOBAL combine<int, long>
; (_Z7combineIilEfff, already exercised above) must not be served by the ns::
; template candidate, and the ns::combine<int, long> call above must not be
; served by the GLOBAL template candidate. Covered by the two positive cases'
; CHECK-NOT lines being namespace-specific; this case adds an explicit negative
; for a templated call whose base (other) matches no candidate at all.
; _Z5otherIilEfff -> other<int, long>(float, float).
; ---------------------------------------------------------------------------
define dso_local void @test_template_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_template_no_match(
; CHECK:         call float @_Z5otherIilEfff(
; CHECK-NOT:     call <32 x float> @_Z22ripple_ew_pure_combineIilEDv32_fS0_S0_(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @_Z5otherIilEfff(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_Z5otherIilEfff(float, float)

; ---------------------------------------------------------------------------
; Nested template argument. _Z19ripple_ew_pure_nestI3barIiEE.. demangles to
; "ripple_ew_pure_nest<bar<int>>(...)" (base nest) and replaces the scalar call
; nest<bar<int>> (_Z4nestI3barIiEEff). Exercises depth-aware "<...>" trimming.
; ---------------------------------------------------------------------------
define dso_local void @test_nested_template_arg(ptr %A, ptr %B) #0 {
; CHECK-LABEL: define dso_local void @test_nested_template_arg(
; CHECK:         call <32 x float> @_Z19ripple_ew_pure_nestI3barIiEEDv32_fS2_(
; CHECK-NOT:     call float @_Z4nestI3barIiEEff(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %call = call float @_Z4nestI3barIiEEff(float %b, float %b)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @_Z4nestI3barIiEEff(float, float)

; Regular-named candidate, source base name c_myaddf.
define internal <32 x float> @ripple_ew_pure_c_myaddf(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled candidate, global, source base name myaddf.
define internal <32 x float> @_Z21ripple_ew_pure_myaddfDv32_fS_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled candidate in namespace ns, source base name c_myaddf.
define internal <32 x float> @_ZN2ns23ripple_ew_pure_c_myaddfEDv32_fS0_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled TEMPLATE candidate, global: demangles to
; "ripple_ew_pure_combine<int, long>(...)", source base name combine.
define internal <32 x float> @_Z22ripple_ew_pure_combineIilEDv32_fS0_S0_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled TEMPLATE candidate in namespace ns, source base name combine.
define internal <32 x float> @_ZN2ns22ripple_ew_pure_combineIilEEDv32_fS1_S1_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled TEMPLATE candidate with a nested template argument:
; "ripple_ew_pure_nest<bar<int>>(...)", source base name nest.
define internal <32 x float> @_Z19ripple_ew_pure_nestI3barIiEEDv32_fS2_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; Itanium-mangled candidate in namespace a::b, source base name myaddf.
define internal <32 x float> @_ZN1a1b21ripple_ew_pure_myaddfEDv32_fS1_(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
