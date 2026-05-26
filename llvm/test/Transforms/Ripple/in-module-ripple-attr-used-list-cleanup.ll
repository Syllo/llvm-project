; Tests that the ripple module pass strips ripple_vector_candidate functions
; from llvm.compiler.used / llvm.used after running. This is the cleanup
; counterpart to clang force-emitting these declarations and pinning them via
; addUsedOrCompilerUsedGlobal so they survive pre-ripple GlobalDCE: once the
; ripple pass has done its work, candidates that were substituted stay alive
; through their direct call references, and unused ones become collectible by
; later DCE passes.

; 1. globaldce alone: candidates are pinned by llvm.compiler.used, so neither
;    declaration is collectible. The pinning entries are still present.
; RUN: opt -passes='globaldce' -S < %s | FileCheck %s --check-prefix=DCE-ONLY

; 2. globaldce + ripple: removed from the @llvm.compiler.used and @llvm.used lists
; RUN: opt -passes='globaldce,ripple' -S < %s | FileCheck %s --check-prefix=DCE-RIPPLE

; 3. globaldce + ripple + globaldce: the second globaldce drops the
; unsubstituted candidate while leaving the substituted one alive (kept by its
; direct call reference).
; RUN: opt -passes='globaldce,ripple,globaldce' -S < %s | FileCheck %s --check-prefix=DCE-RIPPLE-DCE

target datalayout = "e-m:e-p:32:32:32-a:0-n16:32-i64:64:64-i32:32:32-i16:16:16-i1:8:8-f32:32:32-f64:64:64-v32:32:32-v64:64:64-v512:512:512-v1024:1024:1024-v2048:2048:2048"

@llvm.compiler.used = appending global [3 x ptr] [ptr @ripple_ew_pure_myaddf, ptr @ripple_ew_pure_unused, ptr @some_other_used_fn], section "llvm.metadata"
@llvm.used = appending global [3 x ptr] [ptr @ripple_ew_pure_myaddf, ptr @ripple_ew_pure_unused, ptr @some_other_used_fn], section "llvm.metadata"

; DCE-ONLY: @llvm.compiler.used = appending global [3 x ptr] [ptr @ripple_ew_pure_myaddf, ptr @ripple_ew_pure_unused, ptr @some_other_used_fn]
; DCE-RIPPLE: @llvm.compiler.used = appending global [1 x ptr] [ptr @some_other_used_fn]
; DCE-RIPPLE-DCE: @llvm.compiler.used = appending global [1 x ptr] [ptr @some_other_used_fn]

; DCE-ONLY: @llvm.used = appending global [3 x ptr] [ptr @ripple_ew_pure_myaddf, ptr @ripple_ew_pure_unused, ptr @some_other_used_fn]
; DCE-RIPPLE: @llvm.used = appending global [1 x ptr] [ptr @some_other_used_fn]
; DCE-RIPPLE-DCE: @llvm.used = appending global [1 x ptr] [ptr @some_other_used_fn]

define dso_local void @foo(ptr %A, ptr %B, ptr %C) #0 {
; DCE-ONLY-LABEL:        define dso_local void @foo(
; DCE-RIPPLE-LABEL:      define dso_local void @foo(
; DCE-RIPPLE-DCE-LABEL:  define dso_local void @foo(
;
; DCE-ONLY: call float @myaddf(
; DCE-RIPPLE:     call <32 x float> @ripple_ew_pure_myaddf(
; DCE-RIPPLE-DCE: call <32 x float> @ripple_ew_pure_myaddf(
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

declare float @myaddf(float, float)

; DCE-ONLY:        declare <32 x float> @ripple_ew_pure_myaddf(
; DCE-RIPPLE:      declare <32 x float> @ripple_ew_pure_myaddf(
; DCE-RIPPLE-DCE:  declare <32 x float> @ripple_ew_pure_myaddf(
declare <32 x float> @ripple_ew_pure_myaddf(<32 x float>, <32 x float>) #1

; DCE-ONLY:        declare <32 x float> @ripple_ew_pure_unused(
; DCE-RIPPLE:      declare <32 x float> @ripple_ew_pure_unused(
; DCE-RIPPLE-DCE-NOT: @ripple_ew_pure_unused
declare <32 x float> @ripple_ew_pure_unused(<32 x float>) #1

; DCE-ONLY:        declare void @some_other_used_fn(
; DCE-RIPPLE:      declare void @some_other_used_fn(
; DCE-RIPPLE-DCE:  declare void @some_other_used_fn(
declare void @some_other_used_fn()

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #3

attributes #0 = { nounwind "target-features"="-long-calls" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
