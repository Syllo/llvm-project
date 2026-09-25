; RUN: split-file %s %t
; RUN: not opt -passes=ripple -disable-output %t/blocks.ll 2>&1 | FileCheck %s --check-prefix=BLOCKS --implicit-check-not=Assertion
; RUN: not opt -passes=ripple -disable-output %t/native.ll 2>&1 | FileCheck %s --check-prefix=NATIVE --implicit-check-not=Assertion

; Failed specialization requests must produce diagnostics, never wait on an
; empty pending-specialization set. Block-only calls cannot use a scalar loop
; fallback because their block parameters need compile-time resolution.
; BLOCKS: error: {{.*}}Ripple cannot specialize function 'sizes': cannot form a broadcast-compatible mask from the argument and block shapes:
; NATIVE: error: {{.*}}Ripple cannot specialize function 'helper': argument 2 has a varying native vector type

;--- blocks.ll
define i64 @caller() {
  %bs2 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %bs4 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call i64 @sizes(ptr %bs2, ptr %bs4)
  ret i64 %r
}
define internal i64 @sizes(ptr %a, ptr %b) noinline {
  %x = call i64 @llvm.ripple.block.getsize.i64(ptr %a, i64 0)
  %y = call i64 @llvm.ripple.block.getsize.i64(ptr %b, i64 0)
  %r = add i64 %x, %y
  ret i64 %r
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.getsize.i64(ptr, i64 immarg)

;--- native.ll
define void @caller(ptr %in, ptr %out) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %p = getelementptr <2 x i64>, ptr %in, i64 %id
  %v = load <2 x i64>, ptr %p
  call void @helper(ptr %bs, ptr %out, <2 x i64> %v)
  ret void
}
define internal void @helper(ptr %bs, ptr %out, <2 x i64> %v) noinline {
  store <2 x i64> %v, ptr %out
  ret void
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
