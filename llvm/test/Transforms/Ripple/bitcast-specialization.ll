; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not=alloca --implicit-check-not='call {{.*}}@llvm.ripple.bitcast'

; Specialize a helper carrying a shaped scalar argument. Its block is defined
; locally so the unspecialized scalar version can also be lowered (as a splat).
; Exercise both conversions inside the specialized helper.
define <4 x i64> @round_trip(<4 x i64> %v) {
; CHECK-LABEL: define <4 x i64> @round_trip(
; CHECK: [[OUT:%.*]] = call fastcc <4 x i64> @ripple.specialization.final.{{[0-9]+}}.to_native(<4 x i64> %v)
; CHECK-NEXT: ret <4 x i64> [[OUT]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  %out = call <4 x i64> @to_native(i64 %x)
  ret <4 x i64> %out
}

define internal <4 x i64> @to_native(i64 %x) noinline {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  %r = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  %sum = add i64 %r, 1
  %out = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %sum)
  ret <4 x i64> %out
}

; CHECK-LABEL: define internal fastcc <4 x i64> @ripple.specialization.final.{{[0-9]+}}.to_native(<4 x i64> %{{.*}})
; CHECK: [[SUM:%.*]] = add <4 x i64> %{{.*}}, splat (i64 1)
; CHECK-NEXT: ret <4 x i64> [[SUM]]

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
