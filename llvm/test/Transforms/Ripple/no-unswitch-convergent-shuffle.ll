; RUN: opt -passes='loop(simple-loop-unswitch<nontrivial>),verify<loops>' -S < %s | FileCheck %s
; RUN: opt -passes='loop-mssa(simple-loop-unswitch<nontrivial>),verify<loops>' -S -verify-memoryssa < %s | FileCheck %s

; Ripple intrinsics have SIMD semantics that LLVM sees as scalar; cloning
; them before the Ripple pass runs may break program semantics. `convergent`
; (via IntrConvergent in IntrinsicsRipple.td) blocks duplication across any
; transform that would clone instructions — unrolling, jump threading, tail
; duplication, etc. Nontrivial loop unswitching is used here as a convenient
; way to exercise the cloning behavior we must prevent for
; llvm.ripple.shuffle.

declare i32 @llvm.ripple.shuffle.i32(i32, i32, i1 immarg, ptr)
declare i32 @get_src_id(i32)
declare i32 @printf(...)

; CHECK-LABEL: @test(
; CHECK-NOT: br i1 %cond, label %[[#]].split
define i32 @test(i32 %N, i1 %cond) {
entry:
  br label %loop

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
  %s = call i32 @llvm.ripple.shuffle.i32(i32 %v, i32 %v, i1 false, ptr @get_src_id)
  %p = call i32 (...) @printf(i32 %s)
  br label %latch

; CHECK-COUNT-1:         call i32 @llvm.ripple.shuffle.i32
; CHECK-COUNT-1:         call i32 (...) @printf

latch:
  %j.next = add i32 %j, 1
  %done = icmp eq i32 %j.next, %N
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
