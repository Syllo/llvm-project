; REQUIRES: hexagon-registered-target
; RUN: opt -S -passes='module(ripple)' -ripple-pad-to-target-simd -verify-each %s -o - | FileCheck %s

target triple = "hexagon"

; A non-elementwise vector candidate returns a scalar while consuming a
; tensor argument. Padding must narrow the argument to the candidate's ABI
; even though the call has scalar Ripple result shape.
define i64 @pad_scalar_candidate(ptr %in) #0 {
; CHECK-LABEL: define i64 @pad_scalar_candidate(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[LD]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: [[CALL:%.*]] = call i64 @ripple_pure_first_lane(<4 x i64> [[NARROW]])
; CHECK-NEXT: ret i64 [[CALL]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %gep = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %gep
  %r = call i64 @first_lane(i64 %x)
  ret i64 %r
}

; An elementwise candidate also retains its fixed-width argument and result
; ABI. Its tensor-shaped result is re-expanded for the padded store.
define void @pad_vector_candidate(ptr %in, ptr %out) #0 {
; CHECK-LABEL: define void @pad_vector_candidate(
; CHECK-NEXT: [[LD:%.*]] = call <16 x i64> @llvm.masked.load.v16i64.p0(ptr align 8 %in, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>, <16 x i64> poison)
; CHECK-NEXT: [[NARROW:%.*]] = shufflevector <16 x i64> [[LD]], <16 x i64> poison, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
; CHECK-NEXT: [[CALL:%.*]] = call <4 x i64> @ripple_ew_pure_increment(<4 x i64> [[NARROW]])
; CHECK-NEXT: [[WIDEN:%.*]] = shufflevector <4 x i64> [[CALL]], <4 x i64> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison,{{.*}}>
; CHECK-NEXT: call void @llvm.masked.store.v16i64.p0(<16 x i64> [[WIDEN]], ptr align 8 %out, <16 x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,{{.*}}>)
; CHECK-NEXT: ret void
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %in.gep = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %in.gep
  %r = call i64 @increment(i64 %x)
  %out.gep = getelementptr i64, ptr %out, i64 %id
  store i64 %r, ptr %out.gep
  ret void
}

declare i64 @first_lane(i64)
declare i64 @ripple_pure_first_lane(<4 x i64>) #1
declare i64 @increment(i64)
declare <4 x i64> @ripple_ew_pure_increment(<4 x i64>) #1

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64)

attributes #0 = { "target-cpu"="hexagonv81" "target-features"="+hvx-length128b,+hvxv81,+v81" }
attributes #1 = { nounwind "ripple-vector-candidate" }
