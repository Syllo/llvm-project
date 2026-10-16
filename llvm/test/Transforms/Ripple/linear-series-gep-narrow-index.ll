; RUN: opt -passes=ripple -S %s | FileCheck %s --implicit-check-not="warning:"
; RUN: opt -passes=instcombine -S %s | FileCheck %s --check-prefix=RANGE
; RUN: opt -passes='module(function(instcombine),ripple)' -S %s | FileCheck %s --check-prefix=RANGE-RIPPLE

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @llvm.ripple.block.setshape.i32(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32)
declare i8 @llvm.ripple.block.index.i8(ptr, i8)
declare i32 @llvm.ripple.broadcast.i32(ptr, i64, i32)

; GEP sign-extends i32 indices to the i64 index type. The negative slope of
; the index n - v0 must be sign-extended, giving byte offsets 0, -4, -8, -12
; (and not 0, 4 * (2^32 - 1), ...).

; CHECK-LABEL: define void @narrow_index_negative_slope(
; CHECK: %[[N:.*]] = sext i32 %n to i64
; CHECK: %[[BASE:.*]] = getelementptr [4 x i8], ptr %A, i64 %[[N]]
; CHECK: %[[PTRS:.*]] = getelementptr i8, ptr %[[BASE]], <4 x i64> <i64 0, i64 -4, i64 -8, i64 -12>
; CHECK: call void @llvm.masked.scatter.v4f32.v4p0(<4 x float> {{.*}}, <4 x ptr> align 4 %[[PTRS]],
define void @narrow_index_negative_slope(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = sub nsw i32 %n, %v0
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = sitofp i32 %v0 to float
  store float %val, ptr %ptr, align 4
  ret void
}

; Without nsw, n + v0 may wrap in i32 (e.g. n = 2^31 - 2), and
; sext(n + v0) != sext(n) + v0. The index is not folded into the pointer
; series: each lane is sign-extended from the i32 vector index.

; CHECK-LABEL: define void @narrow_index_may_wrap(
; CHECK: %[[IDX:.*]] = add <4 x i32> %{{.*}}, <i32 0, i32 1, i32 2, i32 3>
; CHECK: %[[IDX64:.*]] = sext <4 x i32> %[[IDX]] to <4 x i64>
; CHECK: %[[PTRS:.*]] = getelementptr [4 x i8], ptr %A, <4 x i64> %[[IDX64]]
; CHECK: call void @llvm.masked.scatter.v4f32.v4p0(<4 x float> {{.*}}, <4 x ptr> align 4 %[[PTRS]],
; CHECK-NOT: store <4 x float>
define void @narrow_index_may_wrap(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = add i32 %n, %v0
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = sitofp i32 %v0 to float
  store float %val, ptr %ptr, align 4
  ret void
}

; InstCombine derives no-wrap flags from range metadata. Ripple may then
; distribute the GEP's extension across the resulting linear series.

; RANGE-LABEL: define void @narrow_index_range_proven(
; RANGE: %idx = shl nuw nsw i32 %v0, 1
define void @narrow_index_range_proven(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0), !range !0
  %idx = mul i32 %v0, 2
  %ptr = getelementptr i8, ptr %A, i32 %idx
  store i8 0, ptr %ptr, align 1
  ret void
}

; RANGE-RIPPLE-LABEL: define void @narrow_index_range_proven(
; RANGE-RIPPLE: %ptr.ripple.LS.instance = getelementptr i8, ptr %A, <4 x i64> <i64 0, i64 2, i64 4, i64 6>
; RANGE-RIPPLE: call void @llvm.masked.scatter.v4i8.v4p0(

; With nsw, n + v0 does not wrap and the access stays contiguous.

; CHECK-LABEL: define void @narrow_index_positive_slope(
; CHECK: %[[N:.*]] = sext i32 %n to i64
; CHECK: %[[PTR:.*]] = getelementptr [4 x i8], ptr %A, i64 %[[N]]
; CHECK: store <4 x float> {{.*}}, ptr %[[PTR]], align 4
define void @narrow_index_positive_slope(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = add nsw i32 %n, %v0
  %ptr = getelementptr float, ptr %A, i32 %idx
  %val = sitofp i32 %v0 to float
  store float %val, ptr %ptr, align 4
  ret void
}

; A constant is a non-wrapping leaf.

; CHECK-LABEL: define void @narrow_index_constant(
; CHECK: getelementptr i8, ptr %A, i64 7
; CHECK: store <4 x i8> {{.*}}, ptr {{.*}}, align 1
define void @narrow_index_constant(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %idx = add nsw i32 7, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; A scalar-shaped instruction is a non-wrapping leaf.

; CHECK-LABEL: define void @narrow_index_scalar_instruction(
; CHECK: %[[N:.*]] = sext i32 %n to i64
; CHECK: getelementptr i8, ptr %A, i64 %[[N]]
; CHECK: store <4 x i8> {{.*}}, ptr {{.*}}, align 1
define void @narrow_index_scalar_instruction(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %scalar = add i32 %n, 0
  %idx = add nsw i32 %scalar, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; A vector-shaped broadcast with valid zero slopes is a non-wrapping leaf.

; CHECK-LABEL: define void @narrow_index_zero_slope_instruction(
; CHECK: %[[N:.*]] = sext i32 %n to i64
; CHECK: getelementptr i8, ptr %A, i64 %[[N]]
; CHECK: store <4 x i8> {{.*}}, ptr {{.*}}, align 1
define void @narrow_index_zero_slope_instruction(ptr %A, i32 %n) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %uniform = call i32 @llvm.ripple.broadcast.i32(ptr %BS, i64 1, i32 %n)
  %idx = add nsw i32 %uniform, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; A vector-shaped instruction with non-zero slopes is not a non-wrapping leaf.

; CHECK-LABEL: define void @narrow_index_varying_instruction(
; CHECK: getelementptr i8, ptr %A, <4 x i64> <i64 0, i64 2, i64 4, i64 6>
; CHECK: call void @llvm.masked.scatter.v4i8.v4p0(<4 x i8> {{.*}}, <4 x ptr> align 1 {{.*}}, <4 x i1> {{.*}})
define void @narrow_index_varying_instruction(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %varying = call i32 @llvm.ripple.broadcast.i32(ptr %BS, i64 1, i32 %v0)
  %idx = add nsw i32 %varying, %v0
  %ptr = getelementptr i8, ptr %A, i32 %idx
  %val = trunc i32 %v0 to i8
  store i8 %val, ptr %ptr, align 1
  ret void
}

; A plain block index is a non-wrapping leaf and stays contiguous.

; CHECK-LABEL: define void @narrow_index_block_index(
; CHECK: store <4 x float> {{.*}}, ptr %A, align 4
define void @narrow_index_block_index(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i32 @llvm.ripple.block.index.i32(ptr %BS, i32 0)
  %ptr = getelementptr float, ptr %A, i32 %v0
  %val = sitofp i32 %v0 to float
  store float %val, ptr %ptr, align 4
  ret void
}

; An i8 block index over 128 lanes takes the values [0, 127] and does not wrap.

; CHECK-LABEL: define void @narrow_block_index_fits(
; CHECK: store <128 x i8> {{.*}}, ptr %A, align 1
define void @narrow_block_index_fits(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 128, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i8 @llvm.ripple.block.index.i8(ptr %BS, i8 0)
  %ptr = getelementptr i8, ptr %A, i8 %v0
  store i8 %v0, ptr %ptr, align 1
  ret void
}

; The unsigned proof accepts all 256 values of an i8 block index when it is
; zero-extended explicitly.

; CHECK-LABEL: define void @narrow_block_index_zext_fits(
; CHECK: store <256 x i8> {{.*}}, ptr %A, align 1
define void @narrow_block_index_zext_fits(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 256, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i8 @llvm.ripple.block.index.i8(ptr %BS, i8 0)
  %idx = zext i8 %v0 to i64
  %ptr = getelementptr i8, ptr %A, i64 %idx
  store i8 %v0, ptr %ptr, align 1
  ret void
}

; An i8 block index over 256 lanes wraps in i8 before explicit zero extension.
; CHECK-LABEL: define void @narrow_block_index_zext_wraps(
; CHECK: getelementptr i8, ptr %A, <257 x i64> <i64 0, i64 1, {{.*}}, i64 254, i64 255, i64 0>
; CHECK: call void @llvm.masked.scatter.v257i8.v257p0(<257 x i8> {{.*}}, <257 x ptr> align 1 {{.*}}, <257 x i1> {{.*}})
define void @narrow_block_index_zext_wraps(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 257, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i8 @llvm.ripple.block.index.i8(ptr %BS, i8 0)
  %idx = zext i8 %v0 to i64
  %ptr = getelementptr i8, ptr %A, i64 %idx
  store i8 %v0, ptr %ptr, align 1
  ret void
}

; Over 256 lanes, i8 block indices 128 to 255 are -128 to -1 once
; sign-extended by the GEP, so the access is not contiguous.

; CHECK-LABEL: define void @narrow_block_index_wraps(
; CHECK: %[[PTRS:.*]] = getelementptr i8, ptr %A, <256 x i64> <i64 0, i64 1, {{.*}}, i64 126, i64 127, i64 -128, i64 -127, {{.*}}, i64 -2, i64 -1>
; CHECK: call void @llvm.masked.scatter.v256i8.v256p0(<256 x i8> {{.*}}, <256 x ptr> align 1 %[[PTRS]],
; CHECK-NOT: store <256 x i8>
define void @narrow_block_index_wraps(ptr %A) {
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 256, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %v0 = call i8 @llvm.ripple.block.index.i8(ptr %BS, i8 0)
  %ptr = getelementptr i8, ptr %A, i8 %v0
  store i8 %v0, ptr %ptr, align 1
  ret void
}

!0 = !{i32 0, i32 4}
