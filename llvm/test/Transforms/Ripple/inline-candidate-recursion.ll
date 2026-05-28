; Verify the ripple inliner terminates on mutually-recursive
; ripple-vector-candidate functions. Without a transitive-recursion guard
; the worklist would grow unbounded as inlining A exposes a call to B and
; vice versa. We assert termination (opt returns) and that the entry call
; into the candidate chain gets inlined into the caller. The recursive
; bodies will still contain residual cross-calls, which is acceptable;
; the inliner is not expected to fully unroll mutual recursion.

; RUN: opt -passes="ripple" -S %s | FileCheck %s
; RUN: opt -ripple-run-only-ripple-passes -passes="ripple" -S %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-a:0-n32-S128"
target triple = "hexagon-unknown-elf"

define internal void @ripple_vector_candidate_rec_a(ptr %p, i32 %n) #0 {
entry:
  %z = icmp eq i32 %n, 0
  br i1 %z, label %done, label %rec
rec:
  %nm1 = sub i32 %n, 1
  call void @ripple_vector_candidate_rec_b(ptr %p, i32 %nm1)
  br label %done
done:
  store volatile i32 1, ptr %p, align 4
  ret void
}

define internal void @ripple_vector_candidate_rec_b(ptr %p, i32 %n) #0 {
entry:
  %z = icmp eq i32 %n, 0
  br i1 %z, label %done, label %rec
rec:
  %nm1 = sub i32 %n, 1
  call void @ripple_vector_candidate_rec_a(ptr %p, i32 %nm1)
  br label %done
done:
  store volatile i32 2, ptr %p, align 4
  ret void
}

; The first hop from caller_rec into rec_a must be inlined: the volatile
; store of 1 from rec_a's body should appear in caller_rec.
define void @caller_rec(ptr %p, i32 %n) {
; CHECK-LABEL: @caller_rec(
; CHECK: store volatile i32 1, ptr %p, align 4
; CHECK: ret void
entry:
  call void @ripple_vector_candidate_rec_a(ptr %p, i32 %n)
  ret void
}

attributes #0 = { "ripple-vector-candidate" }
