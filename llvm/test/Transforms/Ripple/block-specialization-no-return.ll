; RUN: opt -passes=ripple -pass-remarks=ripple -verify-each -S %s 2>&1 | FileCheck %s

; Remarks must handle a specialization without any return instruction.
; CHECK: remark: {{.*}}function 'helper' specialized for tensor operands {shape: (unchanged, unchanged, unchanged) -> unchanged}
; CHECK: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.helper(
; CHECK: unreachable

define void @caller(ptr %out, i64 %n) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @helper(ptr %bs, ptr %out, i64 %n)
  ret void
}
define internal void @helper(ptr %bs, ptr %out, i64 %n) noinline noreturn {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %p = getelementptr i64, ptr %out, i64 %id
  store i64 %n, ptr %p
  unreachable
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
