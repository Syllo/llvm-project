; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length128b -hexagon-autohvx=true -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s -D#WIDTH=32
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length64b -hexagon-autohvx=true -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s -D#WIDTH=16

; Padding a variable memory mask must select false in the added lanes.
; An AND with false would leave those lanes poison.
target triple = "hexagon"

; CHECK-LABEL: define void @masked_load_store(
; CHECK: [[LOAD_MASK:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i1> [[PADDED_MASK:%.*]], <[[#WIDTH]] x i1> zeroinitializer
; CHECK: [[VALUE:%.*]] = call <[[#WIDTH]] x i32> @llvm.masked.load.{{.*}}(ptr align 4 %{{.*}}, <[[#WIDTH]] x i1> [[LOAD_MASK]], <[[#WIDTH]] x i32> poison)
; CHECK: [[STORE_MASK:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i1> [[PADDED_MASK]], <[[#WIDTH]] x i1> zeroinitializer
; CHECK: call void @llvm.masked.store.{{.*}}(<[[#WIDTH]] x i32> [[VALUE]], ptr align 4 %{{.*}}, <[[#WIDTH]] x i1> [[STORE_MASK]])

define void @masked_load_store(i32 %n, ptr %in, ptr %out) {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %active = icmp ult i32 %index, %n
  br i1 %active, label %copy, label %exit
copy:
  %src = getelementptr i32, ptr %in, i32 %index
  %value = load i32, ptr %src, align 4
  %dst = getelementptr i32, ptr %out, i32 %index
  store i32 %value, ptr %dst, align 4
  br label %exit
exit:
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
