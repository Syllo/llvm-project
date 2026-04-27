; RUN: opt -passes='loop(simple-loop-unswitch<nontrivial>),verify<loops>' -S < %s | FileCheck %s
; RUN: opt -passes='loop-mssa(simple-loop-unswitch<nontrivial>),verify<loops>' -S -verify-memoryssa < %s | FileCheck %s

; Ripple intrinsics have SIMD semantics that LLVM sees as scalar; cloning
; them before the Ripple pass runs may break program semantics. `convergent`
; (via IntrConvergent in IntrinsicsRipple.td) blocks duplication across any
; transform that would clone instructions — unrolling, jump threading, tail
; duplication, etc. Nontrivial loop unswitching is used here as a convenient
; way to exercise the cloning behavior we must prevent: it would clone the
; loop body on a loop-invariant branch, duplicating the reduction and the
; following printf.

declare i32 @llvm.ripple.reduce.add.i32(i64 immarg, i32)
declare i32 @printf(...)

; CHECK-LABEL: @test(
; No new branch on %cond is created outside the loop.
; CHECK-NOT: br i1 %cond, label %[[#]].split
define i32 @test(i32 %N, i1 %cond) {
entry:
  br label %loop

; The loop-invariant branch stays inside the loop header.
; CHECK:       loop:
; CHECK:         br i1 %cond
loop:
  %j = phi i32 [ 0, %entry ], [ %j.next, %latch ]
  br i1 %cond, label %then, label %else

then:
  br label %merge

else:
  br label %merge

merge:
  %v = phi i32 [ 42, %then ], [ 100, %else ]
  %r = call i32 @llvm.ripple.reduce.add.i32(i64 1, i32 %v)
  %p = call i32 (...) @printf(i32 %r)
  br label %latch

; Exactly one reduce call, exactly one printf call — no cloning.
; CHECK-COUNT-1:         call i32 @llvm.ripple.reduce.add.i32
; CHECK-COUNT-1:         call i32 (...) @printf

latch:
  %j.next = add i32 %j, 1
  %done = icmp eq i32 %j.next, %N
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
