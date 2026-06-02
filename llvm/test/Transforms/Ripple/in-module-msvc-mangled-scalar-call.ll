; RUN: opt -passes='ripple' -S < %s | FileCheck %s --implicit-check-not="warning:"
;
; MSVC-ABI counterpart of in-module-mangled-scalar-call.ll. A whole TU uses one
; ABI, so both the candidate and the scalar call carry MSVC manglings here. The
; pass must demangle each to its source (namespace, base) before matching, which
; requires the SAME namespace and base name. Matrix:
;
;            scalar: regular (c_myaddf)     scalar: mangled (?myaddf@@..)
;   cand                                                                     base
;   regular  ripple_ew_pure_c_myaddf        ripple_ew_pure_c_myaddf          c_myaddf
;   mangled  ?ripple_ew_pure_myaddf@@..     ?ripple_ew_pure_myaddf@@..       myaddf
;
; plus a same-namespace candidate/call pair and a cross-namespace negative case.
; All MSVC names are exactly what clang emits under
; --target=x86_64-pc-windows-msvc (verified with the frontend):
;   ?myaddf@@YAMMM@Z        -> float __cdecl myaddf(float, float)
;   ?c_myaddf@ns@@YAMMM@Z   -> float __cdecl ns::c_myaddf(float, float)
;   ?myaddf@ns@@YAMMM@Z     -> float __cdecl ns::myaddf(float, float)
;   ?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z
;       -> union __m1024 __cdecl ripple_ew_pure_myaddf(union __m1024, union __m1024)
;   ?ripple_ew_pure_c_myaddf@ns@@YAT?$__vector@M$0CA@@__clang@@T23@0@Z
;       -> ...__cdecl ns::ripple_ew_pure_c_myaddf(...)

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
; same-namespace mangled candidate + MSVC-mangled scalar call. ?c_myaddf@ns@@..
; (ns::c_myaddf) matches the candidate ?ripple_ew_pure_c_myaddf@ns@@..
; (ns::ripple_ew_pure_c_myaddf): same namespace ns, base c_myaddf.
; ---------------------------------------------------------------------------
define dso_local void @test_reg_cand_mangled_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_reg_cand_mangled_call(
; CHECK:         call <32 x float> @"?ripple_ew_pure_c_myaddf@ns@@YAT?$__vector@M$0CA@@__clang@@T23@0@Z"(
; CHECK-NOT:     call float @"?c_myaddf@ns@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?c_myaddf@ns@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?c_myaddf@ns@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; MSVC-mangled candidate + regular scalar call (base name myaddf).
; ---------------------------------------------------------------------------
define dso_local void @test_mangled_cand_reg_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_mangled_cand_reg_call(
; CHECK:         call <32 x float> @"?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z"(
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
; MSVC-mangled candidate + MSVC-mangled scalar call (base name myaddf).
; ?myaddf@@YAMMM@Z decodes to "float __cdecl myaddf(float, float)".
; ---------------------------------------------------------------------------
define dso_local void @test_mangled_cand_mangled_call(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_mangled_cand_mangled_call(
; CHECK:         call <32 x float> @"?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z"(
; CHECK-NOT:     call float @"?myaddf@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?myaddf@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?myaddf@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; Negative: an MSVC-mangled scalar call with a DIFFERENT base name
; (?different@@YAMMM@Z -> different) must NOT be replaced by either candidate.
; ---------------------------------------------------------------------------
define dso_local void @test_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_no_match(
; CHECK:         call float @"?different@@YAMMM@Z"(
; CHECK-NOT:     call <32 x float> @ripple_ew_pure_c_myaddf(
; CHECK-NOT:     call <32 x float> @"?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?different@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?different@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; Negative (cross-namespace): ns::myaddf (?myaddf@ns@@YAMMM@Z) shares the base
; name myaddf with the GLOBAL candidate ?ripple_ew_pure_myaddf@@.., but lives in
; namespace ns. Namespaces differ, so it must NOT be replaced.
; ---------------------------------------------------------------------------
define dso_local void @test_cross_namespace_no_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_cross_namespace_no_match(
; CHECK:         call float @"?myaddf@ns@@YAMMM@Z"(
; CHECK-NOT:     call <32 x float> @"?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"?myaddf@ns@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"?myaddf@ns@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; Template instantiation (global). ??$ripple_ew_pure_combine@HJ@@.. demangles to
; "ripple_ew_pure_combine<int, long>(...)" (base combine) once the return type
; and calling convention are suppressed; it replaces the scalar call
; combine<int, long> (??$combine@HJ@@YAMMM@Z).
; ---------------------------------------------------------------------------
define dso_local void @test_template_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_template_match(
; CHECK:         call <32 x float> @"??$ripple_ew_pure_combine@HJ@@YAT?$__vector@M$0CA@@__clang@@T01@0@Z"(
; CHECK-NOT:     call float @"??$combine@HJ@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"??$combine@HJ@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"??$combine@HJ@@YAMMM@Z"(float, float)

; ---------------------------------------------------------------------------
; Template instantiation in namespace ns. ??$ripple_ew_pure_combine@HJ@ns@@..
; (ns::ripple_ew_pure_combine<int, long>, base combine) replaces the scalar call
; ns::combine<int, long> (??$combine@HJ@ns@@YAMMM@Z): same namespace ns.
; ---------------------------------------------------------------------------
define dso_local void @test_namespaced_template_match(ptr %A, ptr %B, ptr %C) #0 {
; CHECK-LABEL: define dso_local void @test_namespaced_template_match(
; CHECK:         call <32 x float> @"??$ripple_ew_pure_combine@HJ@ns@@YAT?$__vector@M$0CA@@__clang@@T12@0@Z"(
; CHECK-NOT:     call float @"??$combine@HJ@ns@@YAMMM@Z"(
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4
  %gep2 = getelementptr inbounds float, ptr %C, i32 %v
  %c = load float, ptr %gep2, align 4
  %call = call float @"??$combine@HJ@ns@@YAMMM@Z"(float %b, float %c)
  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %call, ptr %gep3, align 4
  ret void
}
declare float @"??$combine@HJ@ns@@YAMMM@Z"(float, float)

; Regular-named candidate, source base name c_myaddf.
define internal <32 x float> @ripple_ew_pure_c_myaddf(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; MSVC-mangled candidate, global, source base name myaddf.
define internal <32 x float> @"?ripple_ew_pure_myaddf@@YA?AT__m1024@@T1@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; MSVC-mangled candidate in namespace ns, source base name c_myaddf.
define internal <32 x float> @"?ripple_ew_pure_c_myaddf@ns@@YAT?$__vector@M$0CA@@__clang@@T23@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; MSVC-mangled TEMPLATE candidate, global, source base name combine.
define internal <32 x float> @"??$ripple_ew_pure_combine@HJ@@YAT?$__vector@M$0CA@@__clang@@T01@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

; MSVC-mangled TEMPLATE candidate in namespace ns, source base name combine.
define internal <32 x float> @"??$ripple_ew_pure_combine@HJ@ns@@YAT?$__vector@M$0CA@@__clang@@T12@0@Z"(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
