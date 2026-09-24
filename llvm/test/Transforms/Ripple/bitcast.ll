; RUN: opt -passes=ripple -verify-each -S %s | FileCheck %s --implicit-check-not=alloca --implicit-check-not='call {{.*}}@llvm.ripple.bitcast' --implicit-check-not=llvm.masked

; Native vectors and their Ripple views can have simultaneous users.
define <4 x i64> @shared_views(<4 x i64> %v, ptr %out) {
; CHECK-LABEL: define <4 x i64> @shared_views(
; CHECK: [[SUM:%.*]] = add <4 x i64> %v, splat (i64 1)
; CHECK: store <4 x i64> [[SUM]], ptr %out
; CHECK: [[NATIVE:%.*]] = add <4 x i64> %v, splat (i64 2)
; CHECK: ret <4 x i64> [[NATIVE]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  %sum = add i64 %x, 1
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %sum, ptr %dst
  %native = add <4 x i64> %v, splat (i64 2)
  ret <4 x i64> %native
}

define <4 x i64> @round_trip(<4 x i64> %v) {
; CHECK-LABEL: define <4 x i64> @round_trip(
; CHECK-NEXT: ret <4 x i64> %v
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  %v2 = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v2
}

; Consuming the block shape does not change another use of the Ripple value.
define <4 x i64> @shared_ripple(ptr %in, ptr %out) {
; CHECK-LABEL: define <4 x i64> @shared_ripple(
; CHECK: [[V:%.*]] = load <4 x i64>, ptr %in
; CHECK: [[SUM:%.*]] = add <4 x i64> [[V]], splat (i64 1)
; CHECK: store <4 x i64> [[SUM]], ptr %out
; CHECK: ret <4 x i64> [[V]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %src = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %src
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  %sum = add i64 %x, 1
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %sum, ptr %dst
  ret <4 x i64> %v
}

define <4 x float> @multidimensional(<4 x float> %v) {
; CHECK-LABEL: define <4 x float> @multidimensional(
; CHECK: [[SUM:%.*]] = fadd <4 x float> %v, splat (float 1.000000e+00)
; CHECK: ret <4 x float> [[SUM]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call float @llvm.ripple.from.fixed.vector.v4f32(ptr %bs, <4 x float> %v)
  %sum = fadd float %x, 1.0
  %v2 = call <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float %sum)
  ret <4 x float> %v2
}

define <4 x i64> @broadcast(i64 %x) {
; CHECK-LABEL: define <4 x i64> @broadcast(
; CHECK: insertelement <4 x i64> {{.*}}, i64 %x, i64 0
; CHECK: shufflevector <4 x i64>
; CHECK: ret <4 x i64>
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v
}

define <4 x i64> @constant() {
; CHECK-LABEL: define <4 x i64> @constant(
; CHECK-NEXT: ret <4 x i64> <i64 1, i64 2, i64 3, i64 4>
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> <i64 1, i64 2, i64 3, i64 4>)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v
}

define <1 x i64> @single_lane(<1 x i64> %v) {
; CHECK-LABEL: define <1 x i64> @single_lane(
; CHECK-NEXT: ret <1 x i64> %v
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @llvm.ripple.from.fixed.vector.v1i64(ptr %bs, <1 x i64> %v)
  %v2 = call <1 x i64> @llvm.ripple.to.fixed.vector.v1i64(i64 %x)
  ret <1 x i64> %v2
}

; Materialize a linear series before crossing back to a native vector.
define <4 x i64> @lane_indices() {
; CHECK-LABEL: define <4 x i64> @lane_indices(
; CHECK-NEXT: ret <4 x i64> <i64 0, i64 1, i64 2, i64 3>
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %id)
  ret <4 x i64> %v
}

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
declare float @llvm.ripple.from.fixed.vector.v4f32(ptr, <4 x float>)
declare <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float)
declare i64 @llvm.ripple.from.fixed.vector.v1i64(ptr, <1 x i64>)
declare <1 x i64> @llvm.ripple.to.fixed.vector.v1i64(i64)
