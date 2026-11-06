; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length128b -hexagon-autohvx=true -passes='module(ripple),verify' -ripple-run-only-ripple-passes %s -o - | FileCheck %s --check-prefix=UNPADDED
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length128b -hexagon-autohvx=true -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s --check-prefix=PADDED -D#WIDTH=32
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv75 -mattr=+hvxv75,+hvx-length64b -hexagon-autohvx=true -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - | FileCheck %s --check-prefix=PADDED -D#WIDTH=16

target triple = "hexagon"

; XOR with undef yields undef. Broadcasting must preserve it in every
; original lane, and padding must introduce poison only in added lanes.
; UNPADDED-LABEL: define void @broadcast_undef(
; UNPADDED: store <3 x i32> undef, ptr %{{.*}}, align 4
; PADDED-LABEL: define void @broadcast_undef(
; PADDED: call void @llvm.masked.store.v[[#WIDTH]]i32.p0(<[[#WIDTH]] x i32> <i32 undef, i32 undef, i32 undef, {{(i32 poison, )*}}i32 poison>, ptr align 4 %{{.*}}, <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>)
define void @broadcast_undef(ptr %out) {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %value = xor i32 %index, undef
  %dst = getelementptr i32, ptr %out, i32 %index
  store i32 %value, ptr %dst, align 4
  ret void
}

; Poison must still broadcast and pad to poison rather than becoming undef.
; UNPADDED-LABEL: define void @broadcast_poison(
; UNPADDED: store <3 x i32> poison, ptr %{{.*}}, align 4
; PADDED-LABEL: define void @broadcast_poison(
; PADDED: call void @llvm.masked.store.v[[#WIDTH]]i32.p0(<[[#WIDTH]] x i32> poison, ptr align 4 %{{.*}}, <[[#WIDTH]] x i1> <i1 true, i1 true, i1 true, {{(i1 false, )*}}i1 false>)
define void @broadcast_poison(ptr %out) {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %index = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %value = xor i32 %index, poison
  %dst = getelementptr i32, ptr %out, i32 %index
  store i32 %value, ptr %dst, align 4
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
