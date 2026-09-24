; RUN: split-file %s %t
; RUN: not opt -passes=verify -disable-output %t/to-ripple.ll 2>&1 | FileCheck %s --check-prefix=TO-RIPPLE
; RUN: not opt -passes=verify -disable-output %t/to-vec.ll 2>&1 | FileCheck %s --check-prefix=TO-VEC

; TableGen enforces the element type relationship in both directions.
; TO-RIPPLE: intrinsic return type (vector element of overload type 0) expected i64
; TO-RIPPLE-SAME: but got i32
;--- to-ripple.ll
define i32 @test(ptr %bs, <4 x i64> %v) {
  %x = call i32 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  ret i32 %x
}
declare i32 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)

; TO-VEC: intrinsic argument 0 type (vector element of overload type 0) expected i64
; TO-VEC-SAME: but got i32
;--- to-vec.ll
define <4 x i64> @test(ptr %bs, i32 %x) {
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i32 %x)
  ret <4 x i64> %v
}
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i32)
