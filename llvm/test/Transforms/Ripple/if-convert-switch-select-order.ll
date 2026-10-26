; REQUIRES: hexagon-registered-target
; RUN: opt -mtriple=hexagon -mcpu=hexagonv81 \
; RUN:   -mattr=+hvx-length128b,+hvxv81,+v81 \
; RUN:   -passes='module(ripple),verify' -ripple-pad-to-target-simd \
; RUN:   -ripple-run-only-ripple-passes -disable-output < %s

; A three-way switch requires a cascade of selects when Ripple if-converts
; the vectorized control flow. Keep the selects in dependency order. Inserting
; every select at the block's current first insertion point reverses the
; cascade, leaving a select that uses another select defined later. SIMD
; padding then fails to find the later select in its instruction-order map.

target triple = "hexagon"

define internal i32 @classify(i32 %value) #1 {
entry:
  %shifted = lshr i32 %value, 23
  %kind = trunc i32 %shifted to i8
  switch i8 %kind, label %default [
    i8 0, label %zero
    i8 -1, label %all_ones
  ]

zero:
  br label %exit

all_ones:
  br label %exit

default:
  br label %exit

exit:
  %result = phi i32 [ 0, %zero ], [ -1, %all_ones ], [ %value, %default ]
  ret i32 %result
}

define void @switch_select_order(ptr %input, ptr %output) #0 {
entry:
  %shape = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %shape, i32 0)
  %input.ptr = getelementptr i32, ptr %input, i32 %index
  %value = load i32, ptr %input.ptr, align 4
  %result = call i32 @classify(i32 %value)
  %output.ptr = getelementptr i32, ptr %output, i32 %index
  store i32 %result, ptr %output.ptr, align 4
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv81,+v81" }
attributes #1 = { noinline nounwind "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
