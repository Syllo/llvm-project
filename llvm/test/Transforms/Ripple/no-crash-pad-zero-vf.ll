; REQUIRES: x86-registered-target, asserts
; RUN: opt -S -passes='ripple' -ripple-pad-to-target-simd %s -o - | FileCheck %s

target triple = "i386-unknown-linux-gnu"

define dso_local void @foo(ptr %A) #0 {
; CHECK-LABEL: define dso_local void @foo(
; CHECK: load <33 x i32>
; CHECK: call void @ripple_myvoid(
; CHECK: store <33 x i32>
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 33, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr inbounds i32, ptr %A, i32 %v
  %x = load i32, ptr %p, align 4
  %y = call i32 @myvoid(i32 %x)
  store i32 %y, ptr %p, align 4
  ret void
}

declare i32 @myvoid(i32)
declare void @ripple_myvoid(ptr sret(<33 x i32>) align 4,
                            ptr byval(<33 x i32>) align 4) #1

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) #2
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg) #2

attributes #0 = { nounwind }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { nocallback nofree nosync nounwind willreturn memory(none) }
