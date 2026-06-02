; RUN: opt -passes='ripple' -S < %s 2>&1 | FileCheck %s --implicit-check-not="warning:"
;
; Robustness: SourceFunctionName::parse runs on every scalar call-site symbol in
; the module, so it must never crash or misbehave on the full range of C++
; symbols — operators, ctors/dtors, conversion operators, non-static members,
; compiler-generated special names (vtable/typeinfo), data symbols and outright
; garbage — in both the Itanium and MSVC ABIs. None of these is a ripple
; candidate, so none of the scalar calls below must be replaced, and the pass
; must emit no diagnostics.
;
; All mangled names are real (verified with the frontend / llvm-cxxfilt):
;   _ZN1SclEi          -> S::operator()(int)
;   _ZN1SltERKS_       -> S::operator<(S const&)
;   _ZN1SssERKS_       -> S::operator<=>(S const&)
;   _Znwm              -> operator new(unsigned long)
;   _ZN1ScviEv         -> S::operator int()
;   _ZN1SC1Ev          -> S::S()
;   _ZN1SD1Ev          -> S::~S()
;   _ZN1S6methodEv     -> S::method()       (non-static member; base "method")
;   _ZTV1S / _ZTI1S    -> vtable / typeinfo for S (special names: not functions)
;   ??0S@@QEAA@XZ      -> S::S(void)         (MSVC ctor)
;   ??Bclass@@QEAAHXZ  -> class::operator int(void) (MSVC conversion)
;   ?x@@3HA            -> int x             (MSVC data symbol)

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

; A bona fide global ripple candidate whose source base name is "method". Even
; though the member function _ZN1S6methodEv also has base "method", it lives in
; namespace "S", so it must NOT be matched (member functions are out of scope).
define internal <32 x float> @ripple_ew_pure_method(<32 x float> %a, <32 x float> %b) #1 {
  %r = fadd <32 x float> %a, %b
  ret <32 x float> %r
}

define dso_local void @edge(ptr %A, ptr %B) #0 {
; CHECK-LABEL: define dso_local void @edge(
; The pass must process @edge (it contains ripple intrinsics) without crashing,
; and must leave every odd-named scalar call untouched.
; CHECK-NOT:     call <32 x float> @ripple_ew_pure_method(
; CHECK:         ret void
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %gep = getelementptr inbounds float, ptr %B, i32 %v
  %b = load float, ptr %gep, align 4

  ; Itanium operators / ctor / dtor / conversion / member — none replaced.
  %r1 = call float @_ZN1SclEi(float %b, i32 1)
  %r2 = call float @_ZN1SltERKS_(float %b, ptr %B)
  %r3 = call float @_ZN1SssERKS_(float %b, ptr %B)
  %r4 = call float @_Znwm(i32 4)
  %r5 = call float @_ZN1ScviEv(ptr %B)
  %r6 = call float @_ZN1SC1Ev(ptr %B)
  %r7 = call float @_ZN1SD1Ev(ptr %B)
  %r8 = call float @_ZN1S6methodEv(float %b, float %b)
  ; Special names (vtable/typeinfo) and a garbage _Z symbol used as calls.
  %r9 = call float @_ZTV1S(float %b)
  %r10 = call float @_ZTI1S(float %b)
  %r11 = call float @_ZNStorageGarbageXYZ(float %b)
  ; MSVC ctor / conversion / data-symbol-as-call.
  %r12 = call float @"??0S@@QEAA@XZ"(float %b)
  %r13 = call float @"??Bclass@@QEAAHXZ"(float %b)
  %r14 = call float @"?ridiculous@@@@@@"(float %b)

  %gep3 = getelementptr inbounds float, ptr %A, i32 %v
  store float %r1, ptr %gep3, align 4
  ret void
}

declare float @_ZN1SclEi(float, i32)
declare float @_ZN1SltERKS_(float, ptr)
declare float @_ZN1SssERKS_(float, ptr)
declare float @_Znwm(i32)
declare float @_ZN1ScviEv(ptr)
declare float @_ZN1SC1Ev(ptr)
declare float @_ZN1SD1Ev(ptr)
declare float @_ZN1S6methodEv(float, float)
declare float @_ZTV1S(float)
declare float @_ZTI1S(float)
declare float @_ZNStorageGarbageXYZ(float)
declare float @"??0S@@QEAA@XZ"(float)
declare float @"??Bclass@@QEAAHXZ"(float)
declare float @"?ridiculous@@@@@@"(float)

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
