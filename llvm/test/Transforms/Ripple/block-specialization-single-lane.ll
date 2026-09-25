; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not='call {{.*}}@llvm.ripple'
; RUN: opt -passes='ripple,globaldce' -ripple-run-only-ripple-passes -verify-each -S %s | FileCheck %s --check-prefix=RIPPLE --implicit-check-not='call {{.*}}@llvm.ripple'

; A specialization inherits the caller's active dimensions even when its own
; block has only one lane. It must be finalized, not treated as rank-zero cleanup.
define void @caller(ptr %scalar_out, ptr %vector_out, i64 %x) {
; CHECK-LABEL: define void @caller(
; CHECK: call fastcc void @[[HELPER:ripple.specialization.final.[0-9]+.helper]](ptr poison, ptr %scalar_out, i64 %x)
; CHECK: store <4 x i64> {{.*}}, ptr %vector_out
; CHECK: ret void
  %bs4 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %bs1 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @helper(ptr %bs1, ptr %scalar_out, i64 %x)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs4, i64 0)
  %dst = getelementptr i64, ptr %vector_out, i64 %id
  store i64 %x, ptr %dst
  ret void
}

define internal void @helper(ptr %bs, ptr %out, i64 %x) noinline {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %x, ptr %dst
  ret void
}

; CHECK: define internal fastcc void @[[HELPER]](ptr %0, ptr %1, i64 %2)
; CHECK: store i64 %2, ptr %1
; CHECK: ret void

; Before post-processing folds the zero-index GEP, check that the one-lane
; block index was expanded to the expected constant.
; RIPPLE-LABEL: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.helper(ptr %0, ptr %1, i64 %2)
; RIPPLE: [[DST:%.*]] = getelementptr i64, ptr %1, i64 0
; RIPPLE: store i64 %2, ptr [[DST]]

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
