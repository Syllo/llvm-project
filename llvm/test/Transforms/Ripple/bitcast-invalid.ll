; RUN: split-file %s %t
; RUN: not opt -passes=ripple -verify-each -disable-output %t/too-small.ll 2>&1 | FileCheck %s --check-prefix=TOO-SMALL
; RUN: not opt -passes=ripple -verify-each -disable-output %t/too-large.ll 2>&1 | FileCheck %s --check-prefix=TOO-LARGE
; RUN: not opt -passes=ripple -verify-each -disable-output %t/inverse-width.ll 2>&1 | FileCheck %s --check-prefix=INVERSE-WIDTH
; RUN: not opt -passes=ripple -verify-each -disable-output %t/scalable.ll 2>&1 | FileCheck %s --check-prefix=SCALABLE
; RUN: not opt -passes=ripple -verify-each -disable-output %t/pointer.ll 2>&1 | FileCheck %s --check-prefix=POINTER
; RUN: not opt -passes=ripple -verify-each -disable-output %t/partial-broadcast.ll 2>&1 | FileCheck %s --check-prefix=PARTIAL-BROADCAST
; RUN: not opt -passes=ripple -verify-each -disable-output %t/oversized-splat.ll 2>&1 | FileCheck %s --check-prefix=OVERSIZED-SPLAT
; RUN: not opt -passes=ripple -verify-each -disable-output %t/oversized-active-splat.ll 2>&1 | FileCheck %s --check-prefix=OVERSIZED-ACTIVE-SPLAT
; RUN: not opt -passes=ripple -verify-each -disable-output %t/local-single-lane-to-ripple.ll 2>&1 | FileCheck %s --check-prefix=LOCAL-TO-RIPPLE
; RUN: not opt -passes=ripple -verify-each -disable-output %t/local-single-lane-to-vec.ll 2>&1 | FileCheck %s --check-prefix=LOCAL-TO-VEC
; RUN: opt -passes=ripple -verify-each -S %t/no-block-to-vec.ll | FileCheck %s --check-prefix=NO-BLOCK-TO-VEC
; RUN: not opt -passes=ripple -verify-each -disable-output %t/specialized-single-lane-to-vec.ll 2>&1 | FileCheck %s --check-prefix=SPECIALIZED-TO-VEC
; RUN: not opt -passes=ripple -verify-each -disable-output %t/specialized-single-lane-to-ripple.ll 2>&1 | FileCheck %s --check-prefix=SPECIALIZED-TO-RIPPLE

; TOO-SMALL: Ripple fixed-vector conversion width mismatch: native vector has 256 bits
;--- too-small.ll
define i64 @test(<4 x i64> %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 8, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  ret i64 %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)

; TOO-LARGE: Ripple fixed-vector conversion width mismatch: native vector has 256 bits
;--- too-large.ll
define i64 @test(<4 x i64> %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  ret i64 %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)

; INVERSE-WIDTH: Ripple fixed-vector conversion width mismatch: native vector has 256 bits
;--- inverse-width.ll
define <4 x i64> @test(i64 %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %x = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %id)
  ret <4 x i64> %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)

; OVERSIZED-SPLAT: Ripple scalar fallback cannot broadcast to a native vector with 1073741824 elements; the maximum is 1073741823
;--- oversized-splat.ll
define <1073741824 x i8> @test(ptr %bs, i8 %v) {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %x = call <1073741824 x i8> @llvm.ripple.to.fixed.vector.v1073741824i8(i8 %v)
  ret <1073741824 x i8> %x
}
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare <1073741824 x i8> @llvm.ripple.to.fixed.vector.v1073741824i8(i8)

; OVERSIZED-ACTIVE-SPLAT: Ripple scalar fallback cannot broadcast to a native vector with 1073741824 elements; the maximum is 1073741823
;--- oversized-active-splat.ll
define <1073741824 x i8> @test(i8 %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1073741824, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call <1073741824 x i8> @llvm.ripple.to.fixed.vector.v1073741824i8(i8 %v)
  ret <1073741824 x i8> %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare <1073741824 x i8> @llvm.ripple.to.fixed.vector.v1073741824i8(i8)

; LOCAL-TO-RIPPLE: Ripple fixed-vector conversion width mismatch: native vector has 128 bits, but Ripple value with scalar shape has 32 bits
;--- local-single-lane-to-ripple.ll
define i32 @test(<4 x i32> %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i32 @llvm.ripple.from.fixed.vector.v4i32(ptr %bs, <4 x i32> %v)
  ret i32 %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i32 @llvm.ripple.from.fixed.vector.v4i32(ptr, <4 x i32>)

; LOCAL-TO-VEC: Ripple fixed-vector conversion width mismatch: native vector has 128 bits, but Ripple value with scalar shape has 32 bits
;--- local-single-lane-to-vec.ll
define <4 x i32> @test(i32 %x) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %v = call <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32 %id)
  ret <4 x i32> %v
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
declare <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32)

; A scalar descriptor must still specialize the helper so its conversion is
; validated against that exact one-lane block instead of using fallback cleanup.
; SPECIALIZED-TO-VEC: Ripple fixed-vector conversion width mismatch: native vector has 128 bits, but Ripple value with shape Scalar has 32 bits
;--- specialized-single-lane-to-vec.ll
define <4 x i32> @test(i32 %x) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <4 x i32> @helper(ptr %bs, i32 %x)
  ret <4 x i32> %v
}
define internal <4 x i32> @helper(ptr %bs, i32 %x) noinline {
  %v = call <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32 %x)
  ret <4 x i32> %v
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32)

; SPECIALIZED-TO-RIPPLE: Ripple fixed-vector conversion width mismatch: native vector has 128 bits, but Ripple value with shape Scalar has 32 bits
;--- specialized-single-lane-to-ripple.ll
define i32 @test(<4 x i32> %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i32 @helper(ptr %bs, <4 x i32> %v)
  ret i32 %x
}
define internal i32 @helper(ptr %bs, <4 x i32> %v) noinline {
  %x = call i32 @llvm.ripple.from.fixed.vector.v4i32(ptr %bs, <4 x i32> %v)
  ret i32 %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i32 @llvm.ripple.from.fixed.vector.v4i32(ptr, <4 x i32>)

; An unresolved argument has scalar fallback semantics even without a block
; argument.
; NO-BLOCK-TO-VEC-LABEL: define <4 x i32> @test(
; NO-BLOCK-TO-VEC: [[INSERT:%.*]] = insertelement <4 x i32> poison, i32 %x, i64 0
; NO-BLOCK-TO-VEC: [[SPLAT:%.*]] = shufflevector <4 x i32> [[INSERT]], <4 x i32> poison, <4 x i32> zeroinitializer
; NO-BLOCK-TO-VEC: ret <4 x i32> [[SPLAT]]
;--- no-block-to-vec.ll
define <4 x i32> @test(i32 %x) {
  %v = call <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32 %x)
  ret <4 x i32> %v
}
declare <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32)

; SCALABLE: requires a fixed integer or floating-point vector
;--- scalable.ll
define <vscale x 4 x i64> @test(i64 %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call <vscale x 4 x i64> @llvm.ripple.to.fixed.vector.nxv4i64(i64 %v)
  ret <vscale x 4 x i64> %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare <vscale x 4 x i64> @llvm.ripple.to.fixed.vector.nxv4i64(i64)

; POINTER: requires a fixed integer or floating-point vector
;--- pointer.ll
define <4 x ptr> @test(ptr %v) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call <4 x ptr> @llvm.ripple.to.fixed.vector.v4p0(ptr %v)
  ret <4 x ptr> %x
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare <4 x ptr> @llvm.ripple.to.fixed.vector.v4p0(ptr)

; A smaller tensor must not be implicitly replicated along missing dimensions.
; PARTIAL-BROADCAST: Ripple fixed-vector conversion width mismatch: native vector has 256 bits
;--- partial-broadcast.ll
define <4 x i64> @test() {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %id)
  ret <4 x i64> %v
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
