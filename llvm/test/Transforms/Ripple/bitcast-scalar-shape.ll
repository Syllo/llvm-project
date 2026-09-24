; RUN: opt -passes=ripple -ripple-run-only-ripple-passes -verify-each -S %s | FileCheck %s --implicit-check-not=alloca --implicit-check-not='call {{.*}}@llvm.ripple.bitcast' --implicit-check-not=llvm.masked

; Both the input and result have scalar Ripple shape. The conversion must still
; be processed: splat the input to the result width, then emit a whole-value
; bitcast. Inspect
; IR before post-Ripple InstCombine can fold away the identity bitcast.
define <4 x i64> @scalar_argument(i64 %x) {
; CHECK-LABEL: define <4 x i64> @scalar_argument(
; CHECK-NEXT: [[INSERT:%.*]] = insertelement <4 x i64> poison, i64 %x, i64 0
; CHECK-NEXT: [[SPLAT:%.*]] = shufflevector <4 x i64> [[INSERT]], <4 x i64> poison, <4 x i32> zeroinitializer
; CHECK-NEXT: [[V:%.*]] = bitcast <4 x i64> [[SPLAT]] to <4 x i64>
; CHECK-NEXT: ret <4 x i64> [[V]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v
}

; Broadcast to each conversion's native-vector width.
; The shared scalar input and native-vector results must retain scalar shapes.
define void @different_blocks(i64 %x, ptr %scalar_out, ptr %out4, ptr %out8) {
; CHECK-LABEL: define void @different_blocks(
; CHECK-NEXT: [[SUM:%.*]] = add i64 %x, 1
; CHECK-NEXT: [[INSERT4:%.*]] = insertelement <4 x i64> poison, i64 [[SUM]], i64 0
; CHECK-NEXT: [[SPLAT4:%.*]] = shufflevector <4 x i64> [[INSERT4]], <4 x i64> poison, <4 x i32> zeroinitializer
; CHECK-NEXT: [[V4:%.*]] = bitcast <4 x i64> [[SPLAT4]] to <4 x i64>
; CHECK-NEXT: [[INSERT8:%.*]] = insertelement <8 x i64> poison, i64 [[SUM]], i64 0
; CHECK-NEXT: [[SPLAT8:%.*]] = shufflevector <8 x i64> [[INSERT8]], <8 x i64> poison, <8 x i32> zeroinitializer
; CHECK-NEXT: [[V8:%.*]] = bitcast <8 x i64> [[SPLAT8]] to <8 x i64>
; CHECK-NEXT: store i64 [[SUM]], ptr %scalar_out
; CHECK-NEXT: store <4 x i64> [[V4]], ptr %out4
; CHECK-NEXT: store <8 x i64> [[V8]], ptr %out8
; CHECK-NEXT: ret void
  %bs4 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %bs8 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 8, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %sum = add i64 %x, 1
  %v4 = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %sum)
  %v8 = call <8 x i64> @llvm.ripple.to.fixed.vector.v8i64(i64 %sum)
  store i64 %sum, ptr %scalar_out
  store <4 x i64> %v4, ptr %out4
  store <8 x i64> %v8, ptr %out8
  ret void
}

define <4 x i64> @scalar_constant() {
; CHECK-LABEL: define <4 x i64> @scalar_constant(
; CHECK-NEXT: [[V:%.*]] = bitcast <4 x i64> splat (i64 42) to <4 x i64>
; CHECK-NEXT: ret <4 x i64> [[V]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 42)
  ret <4 x i64> %v
}

define <1 x i64> @single_lane(i64 %x) {
; CHECK-LABEL: define <1 x i64> @single_lane(
; CHECK-NEXT: [[INSERT:%.*]] = insertelement <1 x i64> poison, i64 %x, i64 0
; CHECK-NEXT: [[V:%.*]] = shufflevector <1 x i64> [[INSERT]], <1 x i64> poison, <1 x i32> zeroinitializer
; CHECK-NEXT: ret <1 x i64> [[V]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <1 x i64> @llvm.ripple.to.fixed.vector.v1i64(i64 %x)
  ret <1 x i64> %v
}

; A locally known single-lane block has the same total width, so the conversion
; remains a whole-value bitcast.
define i32 @from_single_lane(<1 x i32> %v) {
; CHECK-LABEL: define i32 @from_single_lane(
; CHECK-NEXT: [[X:%.*]] = bitcast <1 x i32> %v to i32
; CHECK-NEXT: ret i32 [[X]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i32 @llvm.ripple.from.fixed.vector.v1i32(ptr %bs, <1 x i32> %v)
  ret i32 %x
}

; A larger block in the same function selects general vectorization instead of
; single-lane cleanup. The scalar-shaped to.ripple must still be processed.
define i32 @from_single_lane_with_vector_block(<1 x i32> %v, i32 %s, ptr %out) {
; CHECK-LABEL: define i32 @from_single_lane_with_vector_block(
; CHECK-DAG: [[X:%.*]] = bitcast <1 x i32> %v to i32
; CHECK-DAG: [[INSERT:%.*]] = insertelement <4 x i32> poison, i32 %s, i64 0
; CHECK-DAG: [[SPLAT:%.*]] = shufflevector <4 x i32> [[INSERT]], <4 x i32> poison, <4 x i32> zeroinitializer
; CHECK: [[V:%.*]] = bitcast <4 x i32> [[SPLAT]] to <4 x i32>
; CHECK-NEXT: store <4 x i32> [[V]], ptr %out
; CHECK-NEXT: ret i32 [[X]]
  %bs1 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %bs4 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i32 @llvm.ripple.from.fixed.vector.v1i32(ptr %bs1, <1 x i32> %v)
  %outv = call <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32 %s)
  store <4 x i32> %outv, ptr %out
  ret i32 %x
}

; An unspecialized function has no active Ripple dimensions when its block
; comes from a parameter. Extract/splat cleanup keeps its unresolved original
; body well typed; concrete callers use exact specializations.
define <4 x float> @single_lane_opaque_block(ptr %bs, <4 x float> %v) {
; CHECK-LABEL: define <4 x float> @single_lane_opaque_block(
; CHECK-NEXT: [[X:%.*]] = extractelement <4 x float> %v, i64 0
; CHECK-NEXT: [[INSERT:%.*]] = insertelement <4 x float> poison, float [[X]], i64 0
; CHECK-NEXT: [[V:%.*]] = shufflevector <4 x float> [[INSERT]], <4 x float> poison, <4 x i32> zeroinitializer
; CHECK-NEXT: ret <4 x float> [[V]]
  %x = call float @llvm.ripple.from.fixed.vector.v4f32(ptr %bs, <4 x float> %v)
  %result = call <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float %x)
  ret <4 x float> %result
}

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
declare <8 x i64> @llvm.ripple.to.fixed.vector.v8i64(i64)
declare <1 x i64> @llvm.ripple.to.fixed.vector.v1i64(i64)
declare i32 @llvm.ripple.from.fixed.vector.v1i32(ptr, <1 x i32>)
declare <4 x i32> @llvm.ripple.to.fixed.vector.v4i32(i32)
declare i32 @llvm.ripple.from.fixed.vector.v4i32(ptr, <4 x i32>)
declare float @llvm.ripple.from.fixed.vector.v4f32(ptr, <4 x float>)
declare <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float)
