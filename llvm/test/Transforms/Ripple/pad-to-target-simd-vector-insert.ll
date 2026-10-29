; REQUIRES: hexagon-registered-target
; RUN: opt -S -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b,+hvxv79,+v79 -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - 2>&1 | FileCheck %s --implicit-check-not="Unimplemented intrisic" --implicit-check-not="Not padding Ripple generated vectors"

target triple = "hexagon"

; Insert padded subvectors into a padded staging vector, then shuffle only the
; original subvector lanes into the padded parent.

define void @stack_and_pad(ptr %input, ptr %output) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 20, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 1)
  %p0 = getelementptr inbounds i32, ptr %input, i32 %i
  %p1 = getelementptr inbounds i32, ptr %p0, i32 20
  %p2 = getelementptr inbounds i32, ptr %p0, i32 40
  %p3 = getelementptr inbounds i32, ptr %p0, i32 60
  %v0 = load i32, ptr %p0, align 4
  %v1 = load i32, ptr %p1, align 4
  %v2 = load i32, ptr %p2, align 4
  %v3 = load i32, ptr %p3, align 4
  %stack = call i32 (ptr, i32, ...) @llvm.ripple.stack.i32(ptr %bs, i32 %v0, i32 %v1, i32 %v2, i32 %v3)
  %row = mul i32 %j, 20
  %offset = add i32 %row, %i
  %out = getelementptr inbounds i32, ptr %output, i32 %offset
  store i32 %stack, ptr %out, align 4
  ret void
}

; CHECK-LABEL: define void @stack_and_pad(
; CHECK: call <96 x i32> @llvm.vector.insert.v96i32.v32i32
; CHECK: shufflevector <96 x i32> {{.*}}, <96 x i32> {{.*}}, <96 x i32>
; CHECK: call <96 x i32> @llvm.vector.insert.v96i32.v32i32
; CHECK: shufflevector <96 x i32> {{.*}}, <96 x i32> {{.*}}, <96 x i32>
; CHECK: call <96 x i32> @llvm.vector.insert.v96i32.v32i32
; CHECK: shufflevector <96 x i32> {{.*}}, <96 x i32> {{.*}}, <96 x i32>
; CHECK: call <96 x i32> @llvm.vector.insert.v96i32.v32i32
; CHECK: shufflevector <96 x i32> {{.*}}, <96 x i32> {{.*}}, <96 x i32>
; CHECK: call void @llvm.masked.store.v96i32.p0(

; The parent is already SIMD-aligned, but each subvector requires padding:
; <160 x i32>/<20 x i32> becomes <160 x i32>/<32 x i32>.

define void @stack_padded_subvectors(ptr %input, ptr %output) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 20, i32 8, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 1)
  %p0 = getelementptr inbounds i32, ptr %input, i32 %i
  %p1 = getelementptr inbounds i32, ptr %p0, i32 20
  %p2 = getelementptr inbounds i32, ptr %p0, i32 40
  %p3 = getelementptr inbounds i32, ptr %p0, i32 60
  %p4 = getelementptr inbounds i32, ptr %p0, i32 80
  %p5 = getelementptr inbounds i32, ptr %p0, i32 100
  %p6 = getelementptr inbounds i32, ptr %p0, i32 120
  %p7 = getelementptr inbounds i32, ptr %p0, i32 140
  %v0 = load i32, ptr %p0, align 4
  %v1 = load i32, ptr %p1, align 4
  %v2 = load i32, ptr %p2, align 4
  %v3 = load i32, ptr %p3, align 4
  %v4 = load i32, ptr %p4, align 4
  %v5 = load i32, ptr %p5, align 4
  %v6 = load i32, ptr %p6, align 4
  %v7 = load i32, ptr %p7, align 4
  %stack = call i32 (ptr, i32, ...) @llvm.ripple.stack.i32(ptr %bs, i32 %v0, i32 %v1, i32 %v2, i32 %v3, i32 %v4, i32 %v5, i32 %v6, i32 %v7)
  %row = mul i32 %j, 20
  %offset = add i32 %row, %i
  %out = getelementptr inbounds i32, ptr %output, i32 %offset
  store i32 %stack, ptr %out, align 4
  ret void
}

; CHECK-LABEL: define void @stack_padded_subvectors(
; CHECK-NOT: @llvm.vector.insert.v160i32.v20i32
; CHECK-COUNT-8: call <160 x i32> @llvm.vector.insert.v160i32.v32i32
; CHECK: call void @llvm.masked.store.v160i32.p0(

; Accept an already legal vector.insert with the same widths as the reported
; warning without treating the subvector as if it had the parent width.

define void @stack_legal_widths(ptr %input, ptr %output) #0 {
entry:
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 32, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 1)
  %p0 = getelementptr inbounds i32, ptr %input, i32 %i
  %p1 = getelementptr inbounds i32, ptr %p0, i32 32
  %p2 = getelementptr inbounds i32, ptr %p0, i32 64
  %p3 = getelementptr inbounds i32, ptr %p0, i32 96
  %v0 = load i32, ptr %p0, align 4
  %v1 = load i32, ptr %p1, align 4
  %v2 = load i32, ptr %p2, align 4
  %v3 = load i32, ptr %p3, align 4
  %stack = call i32 (ptr, i32, ...) @llvm.ripple.stack.i32(ptr %bs, i32 %v0, i32 %v1, i32 %v2, i32 %v3)
  %row = mul i32 %j, 32
  %offset = add i32 %row, %i
  %out = getelementptr inbounds i32, ptr %output, i32 %offset
  store i32 %stack, ptr %out, align 4
  ret void
}

; CHECK-LABEL: define void @stack_legal_widths(
; CHECK-COUNT-4: call <128 x i32> @llvm.vector.insert.v128i32.v32i32
; CHECK: store <128 x i32>

; A padded parent with an unchanged subvector cannot be produced by
; llvm.ripple.stack. The parent contains an integer number of subvectors, so a
; SIMD-aligned subvector necessarily produces a SIMD-aligned parent.

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
declare i32 @llvm.ripple.stack.i32(ptr, i32, ...)

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
