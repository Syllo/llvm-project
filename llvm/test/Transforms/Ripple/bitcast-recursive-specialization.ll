; RUN: opt -passes=ripple -verify-each -S %s | FileCheck %s --check-prefixes=CHECK,ORIGINAL --implicit-check-not='call {{.*}}@llvm.ripple'
; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not='define internal void @helper' --implicit-check-not='call {{.*}}@llvm.ripple'

; The original helper uses single-lane cleanup despite its unresolved BS;
; the specialized helper uses the full block. GlobalDCE can remove the original.
; CHECK-LABEL: define void @caller(
; CHECK: call fastcc void @[[HELPER:ripple.specialization.final.[0-9]+.helper]](ptr poison, ptr %out, i64 %n)
; ORIGINAL: define internal void @helper(
; ORIGINAL: [[INSERT:%.*]] = insertelement <4 x i64> poison, i64 %n, i64 0
; ORIGINAL: [[V:%.*]] = shufflevector <4 x i64> [[INSERT]], <4 x i64> poison, <4 x i32> zeroinitializer
; ORIGINAL: store <4 x i64> [[V]], ptr %out
; ORIGINAL: call void @helper(
; CHECK: define internal fastcc void @[[HELPER]](
; CHECK: shufflevector <4 x i64>
; CHECK: store <4 x i64>
; CHECK: call fastcc void @[[HELPER]](

define void @caller(ptr %out, i64 %n) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @helper(ptr %bs, ptr %out, i64 %n)
  ret void
}
define internal void @helper(ptr %bs, ptr %out, i64 %n) noinline {
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %n)
  store <4 x i64> %v, ptr %out
  %c = icmp eq i64 %n, 0
  br i1 %c, label %done, label %recurse
recurse:
  %next = sub i64 %n, 1
  call void @helper(ptr %bs, ptr %out, i64 %next)
  br label %done
done:
  ret void
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
