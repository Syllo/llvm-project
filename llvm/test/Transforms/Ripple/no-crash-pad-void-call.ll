; REQUIRES: hexagon-registered-target, asserts
; RUN: opt -passes='ripple' -ripple-pad-to-target-simd -disable-output < %s

target triple = "hexagon"

; The scalar call @myvoid(i32) is replaced by @ripple_myvoid, whose ABI uses
; sret/byval. This creates a void call in vectorized IR that is then padded.

define dso_local void @foo(ptr %A) #0 {
entry:
  %BS = call ptr @llvm.ripple.block.shape.i32(i32 0, i32 33, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %p = getelementptr inbounds i32, ptr %A, i32 %v
  %x = load i32, ptr %p, align 4
  %y = call i32 @myvoid(i32 %x)
  store i32 %y, ptr %p, align 4
  ret void
}

declare i32 @myvoid(i32)
declare void @ripple_myvoid(ptr sret(<33 x i32>) align 4,
                            ptr byval(<33 x i32>) align 4) #1

declare ptr @llvm.ripple.block.shape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
