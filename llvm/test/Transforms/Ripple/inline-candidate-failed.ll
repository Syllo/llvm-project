; Verify that when getInlineCost approves inlining a ripple-vector-candidate
; but InlineFunction rejects it (here: incompatible personality functions), a
; CandidateInlineFailed OptimizationRemarkMissed is emitted.

; RUN: opt -passes="ripple" -S -pass-remarks-missed=ripple %s 2>&1 | FileCheck %s
; RUN: opt -ripple-run-only-ripple-passes -passes="ripple" -S -pass-remarks-missed=ripple %s 2>&1 | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-a:0-n32-S128"
target triple = "hexagon-unknown-elf"

declare i32 @personality_a(...)
declare i32 @personality_b(...)

; alwaysinline makes getInlineCost return "always inline"; incompatible
; personality functions make InlineFunction refuse, triggering the remark.
define internal void @ripple_vector_candidate_pers(ptr %p) #0 personality ptr @personality_a !dbg !3 {
entry:
  store i32 7, ptr %p, align 4
  ret void
}

; CHECK: remark: inline-candidate-failed.c:11:3: could not inline ripple-vector-candidate 'ripple_vector_candidate_pers' into 'caller_pers'
define void @caller_pers(ptr %p) personality ptr @personality_b !dbg !4 {
entry:
  call void @ripple_vector_candidate_pers(ptr %p), !dbg !5
  ret void
}

attributes #0 = { alwaysinline "ripple-vector-candidate" }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1, !2}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !6, isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !{i32 2, !"Dwarf Version", i32 4}
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "ripple_vector_candidate_pers", scope: !6, file: !6, line: 5, type: !7, spFlags: DISPFlagDefinition, unit: !0)
!4 = distinct !DISubprogram(name: "caller_pers", scope: !6, file: !6, line: 10, type: !7, spFlags: DISPFlagDefinition, unit: !0)
!5 = !DILocation(line: 11, column: 3, scope: !4)
!6 = !DIFile(filename: "inline-candidate-failed.c", directory: "/tmp")
!7 = !DISubroutineType(types: !8)
!8 = !{null}
