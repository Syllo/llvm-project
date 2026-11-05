; REQUIRES: hexagon-registered-target
; RUN: opt -S -passes='module(ripple),verify' -ripple-pad-to-target-simd \
; RUN:   -ripple-run-only-ripple-passes %s -o - | FileCheck %s

; Byte operations require padding, which propagates to the half PHI even
; though its 64 lanes already fill HVX. Its native-vector boundary must use
; the rebuilt PHI before the old one is deleted.

target triple = "hexagon"

define <64 x half> @aligned_boundary_phi(<64 x half> %v, ptr %input) #0 {
; CHECK-LABEL: define <64 x half> @aligned_boundary_phi(
; CHECK: [[RESULT:%.*]] = phi <64 x half>
; CHECK-NEXT: [[OUT:%.*]] = bitcast <64 x half> [[RESULT]] to <64 x half>
; CHECK-NEXT: ret <64 x half> [[OUT]]
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 64, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %x = call half @llvm.ripple.from.fixed.vector.v64f16(ptr %bs, <64 x half> %v)
  %cmp = fcmp uno half %x, %x
  br i1 %cmp, label %left, label %right

left:
  %idx = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr i8, ptr %input, i32 %idx
  %byte = load i8, ptr %p
  %short = zext i8 %byte to i16
  %a = bitcast i16 %short to half
  br label %exit

right:
  %b = fsub half %x, 0xH3C00
  br label %exit

exit:
  %r = phi half [ %a, %left ], [ %b, %right ]
  %out = call <64 x half> @llvm.ripple.to.fixed.vector.v64f16(half %r)
  ret <64 x half> %out
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)
declare half @llvm.ripple.from.fixed.vector.v64f16(ptr, <64 x half>)
declare <64 x half> @llvm.ripple.to.fixed.vector.v64f16(half)

attributes #0 = { "target-cpu"="hexagonv81" "target-features"="+hvx-length128b,+hvxv81,+v81" }
