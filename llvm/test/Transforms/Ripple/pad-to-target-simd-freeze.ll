; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple)' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - 2>&1 | FileCheck %s --implicit-check-not="Unimplemented instruction" --implicit-check-not="Not padding Ripple generated vectors"

; Verify that a scalar freeze in a Ripple region is vectorized and then padded
; to the target SIMD width.

target triple = "hexagon"

define dso_local void @freeze_ripple(ptr %output) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %frozen = freeze i32 %index
  %ptr = getelementptr inbounds i32, ptr %output, i32 %index
  store i32 %frozen, ptr %ptr, align 4
  ret void
}

; CHECK-LABEL: define dso_local void @freeze_ripple(
; CHECK: %[[FROZEN:[^ ]+]] = freeze <32 x i32>
; CHECK: call void @llvm.masked.store.v32i32.p0(<32 x i32> %[[FROZEN]],

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #1
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #1

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(none) }
