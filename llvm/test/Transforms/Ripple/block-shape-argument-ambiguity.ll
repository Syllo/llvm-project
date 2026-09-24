; RUN: not opt -passes=ripple -disable-output %s 2>&1 | FileCheck %s

; A block-shape argument and an unresolved pointer both reach the lookup. The
; argument is a known source, so the unresolved clobber makes the access
; ambiguous rather than merely unresolved.
;
; CHECK: error: {{.*}}block shape access is ambiguous (multiple shapes apply)
; CHECK: note: {{.*}}can come from here
; CHECK: note: {{.*}}and here
define i64 @argument_and_unresolved(ptr %argument, i1 %cond) {
entry:
  %slot = alloca ptr
  %active = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %active.index = call i64 @llvm.ripple.block.index.i64(ptr %active, i64 0)
  br i1 %cond, label %from.argument, label %from.unresolved

from.argument:
  store ptr %argument, ptr %slot
  br label %merge

from.unresolved:
  %unresolved = getelementptr i8, ptr %argument, i64 1
  store ptr %unresolved, ptr %slot
  br label %merge

merge:
  %block = load ptr, ptr %slot
  %index = call i64 @llvm.ripple.block.index.i64(ptr %block, i64 0)
  %result = add i64 %active.index, %index
  ret i64 %result
}

declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
