; RUN: opt -passes='loop(simple-loop-unswitch<nontrivial>),verify<loops>' -S < %s | FileCheck %s
; RUN: opt -passes='loop-mssa(simple-loop-unswitch<nontrivial>),verify<loops>' -S -verify-memoryssa < %s | FileCheck %s

; Reinterpret has cross-lane semantics before the Ripple pass.  Convergent
; blocks transforms such as nontrivial loop unswitching from cloning it.
declare i8 @llvm.ripple.reinterp.i8.i32(ptr, i32)
declare i32 @printf(...)

; CHECK-LABEL: @test(
; CHECK-NOT: br i1 %cond, label %[[#]].split
define i32 @test(i32 %N, i1 %cond, ptr %BS) {
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
  %r = call i8 @llvm.ripple.reinterp.i8.i32(ptr %BS, i32 %v)
  %p = call i32 (...) @printf(i8 %r)
  br label %latch

; CHECK-COUNT-1:         call i8 @llvm.ripple.reinterp.i8.i32
; CHECK-COUNT-1:         call i32 (...) @printf

latch:
  %j.next = add i32 %j, 1
  %done = icmp eq i32 %j.next, %N
  br i1 %done, label %exit, label %loop

exit:
  ret i32 0
}
