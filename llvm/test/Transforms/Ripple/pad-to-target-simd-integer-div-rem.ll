; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length128b -passes="module(ripple)" -hexagon-autohvx=true -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s -D#WIDTH=32
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length64b -passes="module(ripple)" -hexagon-autohvx=true -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s -D#WIDTH=16

target triple = "hexagon"

define void @pad_sdiv(ptr %a, ptr %b, ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr i32, ptr %a, i32 %i
  %x = load i32, ptr %pa, align 4
  %pb = getelementptr i32, ptr %b, i32 %i
  %y = load i32, ptr %pb, align 4
  %r = sdiv i32 %x, %y
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}

define void @pad_udiv(ptr %a, ptr %b, ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr i32, ptr %a, i32 %i
  %x = load i32, ptr %pa, align 4
  %pb = getelementptr i32, ptr %b, i32 %i
  %y = load i32, ptr %pb, align 4
  %r = udiv i32 %x, %y
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}

define void @pad_srem(ptr %a, ptr %b, ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr i32, ptr %a, i32 %i
  %x = load i32, ptr %pa, align 4
  %pb = getelementptr i32, ptr %b, i32 %i
  %y = load i32, ptr %pb, align 4
  %r = srem i32 %x, %y
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}

define void @pad_urem(ptr %a, ptr %b, ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr i32, ptr %a, i32 %i
  %x = load i32, ptr %pa, align 4
  %pb = getelementptr i32, ptr %b, i32 %i
  %y = load i32, ptr %pb, align 4
  %r = urem i32 %x, %y
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}

define void @pad_constant_divisor(ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %x = add i32 %i, 256
  %r = sdiv i32 %x, 20
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
attributes #0 = { nounwind }

; CHECK-LABEL: define void @pad_sdiv(
; CHECK: [[LHS:%.*]] = call <[[#WIDTH]] x i32> @llvm.masked.load.
; CHECK: [[RHS:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i32> %{{.*}}, <[[#WIDTH]] x i32> splat (i32 1)
; CHECK-NEXT: {{%.*}} = sdiv <[[#WIDTH]] x i32> [[LHS]], [[RHS]]

; CHECK-LABEL: define void @pad_udiv(
; CHECK: [[LHS:%.*]] = call <[[#WIDTH]] x i32> @llvm.masked.load.
; CHECK: [[RHS:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i32> %{{.*}}, <[[#WIDTH]] x i32> splat (i32 1)
; CHECK-NEXT: {{%.*}} = udiv <[[#WIDTH]] x i32> [[LHS]], [[RHS]]

; CHECK-LABEL: define void @pad_srem(
; CHECK: [[LHS:%.*]] = call <[[#WIDTH]] x i32> @llvm.masked.load.
; CHECK: [[RHS:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i32> %{{.*}}, <[[#WIDTH]] x i32> splat (i32 1)
; CHECK-NEXT: {{%.*}} = srem <[[#WIDTH]] x i32> [[LHS]], [[RHS]]

; CHECK-LABEL: define void @pad_urem(
; CHECK: [[LHS:%.*]] = call <[[#WIDTH]] x i32> @llvm.masked.load.
; CHECK: [[RHS:%.*]] = select <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>, <[[#WIDTH]] x i32> %{{.*}}, <[[#WIDTH]] x i32> splat (i32 1)
; CHECK-NEXT: {{%.*}} = urem <[[#WIDTH]] x i32> [[LHS]], [[RHS]]

; CHECK-LABEL: define void @pad_constant_divisor(
; CHECK: [[NUMERATOR:%.*]] = add <[[#WIDTH]] x i32>
; CHECK: sdiv <[[#WIDTH]] x i32> [[NUMERATOR]], <i32 20, i32 20, i32 20, {{(i32 1, )*}}i32 1>

; CHECK-LABEL: define void @unpad_udiv(
; CHECK-NOT: .safe.
; CHECK: udiv <32 x i32> %{{.*}}, %{{.*}}
; CHECK-NOT: .safe.
; CHECK: ret void

define void @unpad_udiv(ptr %a, ptr %b, ptr %out) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %pa = getelementptr i32, ptr %a, i32 %i
  %x = load i32, ptr %pa, align 4
  %pb = getelementptr i32, ptr %b, i32 %i
  %y = load i32, ptr %pb, align 4
  %r = udiv i32 %x, %y
  %po = getelementptr i32, ptr %out, i32 %i
  store i32 %r, ptr %po, align 4
  ret void
}
