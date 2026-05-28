; Verify the ripple inliner terminates and behaves reasonably on a
; self-recursive ripple-vector-candidate. getInlineCost rejects inlining
; recursive callees, so the candidate body and the call site into it are
; both preserved. The point of this test is that the worklist does not
; loop and the pass does not assert.

; RUN: opt -passes="ripple" -S %s | FileCheck %s
; RUN: opt -ripple-run-only-ripple-passes -passes="ripple" -S %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-a:0-n32-S128"
target triple = "hexagon-unknown-elf"

; CHECK: define internal void @ripple_vector_candidate_self(
; CHECK: call void @ripple_vector_candidate_self(
define internal void @ripple_vector_candidate_self(ptr %p, i32 %n) #0 {
entry:
  %z = icmp eq i32 %n, 0
  br i1 %z, label %done, label %rec
rec:
  %nm1 = sub i32 %n, 1
  call void @ripple_vector_candidate_self(ptr %p, i32 %nm1)
  br label %done
done:
  store volatile i32 7, ptr %p, align 4
  ret void
}

define void @caller_self(ptr %p, i32 %n) {
; CHECK-LABEL: @caller_self(
; CHECK: call void @ripple_vector_candidate_self(
; CHECK: ret void
entry:
  call void @ripple_vector_candidate_self(ptr %p, i32 %n)
  ret void
}

attributes #0 = { "ripple-vector-candidate" }
