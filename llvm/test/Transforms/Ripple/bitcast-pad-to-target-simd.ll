; REQUIRES: hexagon-registered-target
; RUN: opt -S -passes="module(ripple)" -ripple-pad-to-target-simd -verify-each %s -o - | FileCheck %s
; RUN: opt -disable-output -passes="module(ripple)" -ripple-pad-to-target-simd \
; RUN:     -pass-remarks-missed=ripple %s 2>&1 | FileCheck %s --check-prefix=REMARK

; SIMD padding has to stop at a native-vector bitcast. The <4 x i64> view is
; pinned by the user's C vector type and the HVX register class, so it cannot be
; rewritten to the legal <16 x i64> width: the tensor side is padded and narrowed
; back at the boundary instead. Before that, each of these three shapes crashed
; padToTargetSIMDWidth in a different way.

target triple = "hexagon"

; One boundary per direction, and the native side is a function Argument. With
; both ends pinned there is nothing to legalize, so no padded width survives.
define <4 x i64> @pad_native_argument(<4 x i64> %v) #0 {
; CHECK-LABEL: define <4 x i64> @pad_native_argument(
; CHECK-NEXT: [[SUM:%.*]] = add <4 x i64> %v, splat (i64 1)
; CHECK-NEXT: ret <4 x i64> [[SUM]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  %add = add i64 %x, 1
  %r = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %add)
  ret <4 x i64> %r
}

; Same, but the native side is an instruction rather than an Argument.
define <4 x i64> @pad_native_instruction(<4 x i64> %a, <4 x i64> %b) #0 {
; CHECK-LABEL: define <4 x i64> @pad_native_instruction(
; CHECK-NEXT: [[MUL:%.*]] = mul <4 x i64> %a, %b
; CHECK-NEXT: [[SUM:%.*]] = add <4 x i64> [[MUL]], splat (i64 2)
; CHECK-NEXT: ret <4 x i64> [[SUM]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %mul = mul <4 x i64> %a, %b
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %mul)
  %add = add i64 %x, 2
  %r = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %add)
  ret <4 x i64> %r
}

; Only a llvm.ripple.to.fixed.vector boundary, with paddable tensor work
; feeding it. The load and multiply are widened to the legal HVX width, and
; only the final conversion is narrowed back to the native width.
define <4 x i64> @pad_to_vec_only(ptr %in) #0 {
; CHECK-LABEL: define <4 x i64> @pad_to_vec_only(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[MUL:%.*]] = mul <16 x i64> [[LD]], <i64 3, i64 3, i64 3, i64 3, i64 poison,{{.*}}>
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[MUL]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: ret <4 x i64> [[NARROW]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %gep = getelementptr inbounds i64, ptr %in, i64 %id
  %ld = load i64, ptr %gep
  %mul = mul i64 %ld, 3
  %r = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %mul)
  ret <4 x i64> %r
}

; A specialized helper may return a native vector while consuming a
; vector-shaped Ripple argument. Its block argument makes the native-vector
; width explicit. The call has scalar Ripple shape, but padding must still
; narrow the tensor argument back to the helper's fixed-width ABI.
define <4 x i64> @pad_specialized_to_vec(ptr %in) #0 {
; CHECK-LABEL: define <4 x i64> @pad_specialized_to_vec(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[LD]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: [[CALL:%.*]] = call fastcc <4 x i64> @[[TO_NATIVE:ripple.specialization.final.[0-9]+.to_native]](ptr poison, <4 x i64> [[NARROW]])
; CHECK-NEXT: ret <4 x i64> [[CALL]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %gep = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %gep
  %out = call <4 x i64> @to_native(ptr %bs, i64 %x)
  ret <4 x i64> %out
}

define internal <4 x i64> @to_native(ptr %bs, i64 %x) noinline #0 {
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v
}

; Scalar-returning specialized calls need the same fixed-width argument
; narrowing as vector-returning calls. Pass the block through because the
; helper contains a native-vector conversion.
define i64 @pad_specialized_scalar_return(ptr %in) #0 {
; CHECK-LABEL: define i64 @pad_specialized_scalar_return(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[LD]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: [[CALL:%.*]] = call fastcc i64 @[[FIRST_LANE:ripple.specialization.final.[0-9]+.first_lane]](ptr poison, <4 x i64> [[NARROW]])
; CHECK-NEXT: ret i64 [[CALL]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %gep = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %gep
  %out = call i64 @first_lane(ptr %bs, i64 %x)
  ret i64 %out
}

define internal i64 @first_lane(ptr %bs, i64 %x) noinline #0 {
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  %first = extractelement <4 x i64> %v, i64 0
  ret i64 %first
}

; Void specialized calls also need their tensor arguments narrowed even though
; they have no result to remap.
define void @pad_specialized_void_return(ptr %in) #0 {
; CHECK-LABEL: define void @pad_specialized_void_return(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[LD]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: call fastcc void @[[SINK:ripple.specialization.final.[0-9]+.sink]](<4 x i64> [[NARROW]])
; CHECK-NEXT: ret void
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %gep = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %gep
  call void @sink(i64 %x)
  ret void
}

define internal void @sink(i64 %x) noinline #0 {
  ret void
}

; CHECK: define internal fastcc void @[[SINK]](
; CHECK-NEXT: ret void
; CHECK: define internal fastcc i64 @[[FIRST_LANE]](
; CHECK-NEXT: [[FIRST:%.*]] = extractelement <4 x i64> %1, i64 0
; CHECK-NEXT: ret i64 [[FIRST]]
; CHECK: define internal fastcc <4 x i64> @[[TO_NATIVE]](
; CHECK-NEXT: ret <4 x i64> %1

; One remark per boundary: two each for the round trips, one for the
; llvm.ripple.to.fixed.vector conversion.
; REMARK-COUNT-5: remark: {{.*}}SIMD padding stops at a native vector boundary: this 4-lane conversion is not widened to 16 lanes because the native vector type is fixed by the target
; REMARK-NOT: remark: {{.*}}native vector boundary

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)

attributes #0 = { "target-cpu"="hexagonv81" "target-features"="+hvx-length128b,+hvxv81,+v81" }
