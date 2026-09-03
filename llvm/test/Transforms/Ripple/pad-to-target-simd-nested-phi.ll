; REQUIRES: asserts, hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple)' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes < %s | FileCheck %s

; Padding a 16-lane specialization to the legal HVX width creates a padded
; select from the if-converted PHI. The specialization mask is applied before
; padding, so its temporary placeholder must survive until final cloning
; replaces it with the masked specialization's argument.

; CHECK-LABEL: define{{.*}} @masked.ripple.specialization.final
; CHECK-SAME: <16 x i1> [[MASK:%[0-9]+]])
; CHECK: shufflevector <16 x i1> [[MASK]], <16 x i1> poison, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison, i32 poison>

target triple = "hexagon"

define internal float @abs_value(float %x) #1 {
entry:
  %negative = fcmp olt float %x, 0.000000e+00
  br i1 %negative, label %neg, label %exit

neg:
  %negated = fneg float %x
  br label %exit

exit:
  %result = phi float [ %x, %entry ], [ %negated, %neg ]
  ret float %result
}

define void @foo(ptr %a) #0 {
entry:
  %shape = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 16, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %shape, i32 0)
  %ptr = getelementptr float, ptr %a, i32 %index
  %value = load float, ptr %ptr, align 4
  %result = call float @abs_value(float %value)
  store float %result, ptr %ptr, align 4
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
