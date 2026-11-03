; RUN: opt -S -passes='module(ripple),verify' -ripple-run-only-ripple-passes %s -o - | FileCheck %s

; A single-lane destination has scalar Ripple shape. Reinterpret must lower
; the two source lanes to i32, allowing the scalar return to use the result.
define i32 @vector_to_scalar_reinterpret(ptr %input) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %r = call i32 @llvm.ripple.reinterp.u32.i16(ptr %dst, i16 %v)
  ret i32 %r
}
; CHECK-LABEL: define i32 @vector_to_scalar_reinterpret(
; CHECK: [[LOAD:%.*]] = load <2 x i16>,
; CHECK: [[SCALAR:%.*]] = bitcast <2 x i16> [[LOAD]] to i32
; CHECK: ret i32 [[SCALAR]]

; Scalar-to-scalar reinterpret must also produce a scalar LLVM value, which
; can be returned directly and broadcast into varying work.
define float @scalar_to_scalar_reinterpret(i32 %bits, ptr %input, ptr %output) #0 {
  %scalar = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %r = call float @llvm.ripple.reinterp.f32.i32(ptr %scalar, i32 %bits)
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr float, ptr %input, i32 %i
  %v = load float, ptr %p, align 4
  %sum = fadd float %v, %r
  %q = getelementptr float, ptr %output, i32 %i
  store float %sum, ptr %q, align 4
  ret float %r
}
; CHECK-LABEL: define float @scalar_to_scalar_reinterpret(
; CHECK: [[SCALAR:%.*]] = bitcast i32 %bits to float
; CHECK: [[INSERT:%.*]] = insertelement <19 x float> poison, float [[SCALAR]], i64 0
; CHECK: [[SPLAT:%.*]] = shufflevector <19 x float> [[INSERT]],
; CHECK: [[SUM:%.*]] = fadd <19 x float> {{%.*}}, [[SPLAT]]
; CHECK: store <19 x float> [[SUM]],
; CHECK: ret float [[SCALAR]]

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
declare i32 @llvm.ripple.reinterp.u32.i16(ptr, i16)
declare float @llvm.ripple.reinterp.f32.i32(ptr, i32)
attributes #0 = { nounwind }
