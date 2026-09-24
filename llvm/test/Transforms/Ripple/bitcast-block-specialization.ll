; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not=alloca --implicit-check-not='call {{.*}}@llvm.ripple.bitcast'

define <4 x i64> @round_trip_with_block(<4 x i64> %v) {
; CHECK-LABEL: define <4 x i64> @round_trip_with_block(<4 x i64> %v) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[FROM_NATIVE_1:ripple.specialization.final.[0-9]+.from_native]](ptr poison, <4 x i64> %{{[-a-zA-Z$._0-9]+}})
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[TO_NATIVE_1:ripple.specialization.final.[0-9]+.to_native]](ptr poison, <4 x i64> %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @from_native(ptr %bs, <4 x i64> %v)
  %out = call <4 x i64> @to_native(ptr %bs, i64 %x)
  ret <4 x i64> %out
}

define <4 x i64> @uniform_to_native(i64 %x) {
; CHECK-LABEL: define <4 x i64> @uniform_to_native(i64 %x) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[TO_NATIVE_2:ripple.specialization.final.[0-9]+.to_native]](ptr poison, i64 %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %out = call <4 x i64> @to_native(ptr %bs, i64 %x)
  ret <4 x i64> %out
}

define <4 x i64> @constant_native() {
; CHECK-LABEL: define <4 x i64> @constant_native() {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[FROM_NATIVE_1]](ptr poison, <4 x i64> splat (i64 7))
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[TO_NATIVE_1]](ptr poison, <4 x i64> %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @from_native(ptr %bs, <4 x i64> splat (i64 7))
  %out = call <4 x i64> @to_native(ptr %bs, i64 %x)
  ret <4 x i64> %out
}

; A scalar block still requests an exact specialization, so the conversions are
; validated against its one-lane shape.
define i32 @single_lane_round_trip(i32 %x) {
; CHECK-LABEL: define i32 @single_lane_round_trip(i32 %x) {
; CHECK-NEXT: [[V:%.*]] = call fastcc <1 x i32> @[[TO_SINGLE_NATIVE:ripple.specialization.final.[0-9]+.to_single_native]](ptr poison, i32 %x)
; CHECK-NEXT: [[OUT:%.*]] = call fastcc i32 @[[FROM_SINGLE_NATIVE:ripple.specialization.final.[0-9]+.from_single_native]](ptr poison, <1 x i32> [[V]])
; CHECK-NEXT: ret i32 [[OUT]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v = call <1 x i32> @to_single_native(ptr %bs, i32 %x)
  %out = call i32 @from_single_native(ptr %bs, <1 x i32> %v)
  ret i32 %out
}

define internal i64 @from_native(ptr %bs, <4 x i64> %v) noinline {
  %x = call i64 @llvm.ripple.from.fixed.vector.v4i64(ptr %bs, <4 x i64> %v)
  ret i64 %x
}

define internal <4 x i64> @to_native(ptr %bs, i64 %x) noinline {
  %v = call <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64 %x)
  ret <4 x i64> %v
}

define internal <1 x i32> @to_single_native(ptr %bs, i32 %x) noinline {
  %v = call <1 x i32> @llvm.ripple.to.fixed.vector.v1i32(i32 %x)
  ret <1 x i32> %v
}

define internal i32 @from_single_native(ptr %bs, <1 x i32> %v) noinline {
  %x = call i32 @llvm.ripple.from.fixed.vector.v1i32(ptr %bs, <1 x i32> %v)
  ret i32 %x
}

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.getsize.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.from.fixed.vector.v4i64(ptr, <4 x i64>)
declare <4 x i64> @llvm.ripple.to.fixed.vector.v4i64(i64)
declare i32 @llvm.ripple.from.fixed.vector.v1i32(ptr, <1 x i32>)
declare <1 x i32> @llvm.ripple.to.fixed.vector.v1i32(i32)

; CHECK: define internal fastcc <1 x i32> @[[TO_SINGLE_NATIVE]](ptr %0, i32 %1) {{.*}}{
; CHECK: [[INSERT:%.*]] = insertelement <1 x i32> poison, i32 %1, i64 0
; CHECK: ret <1 x i32> [[INSERT]]

; CHECK: define internal fastcc i32 @[[FROM_SINGLE_NATIVE]](ptr %0, <1 x i32> %1) {{.*}}{
; CHECK: [[EXTRACT:%.*]] = extractelement <1 x i32> %1, i64 0
; CHECK: ret i32 [[EXTRACT]]

; CHECK: define internal fastcc <4 x i64> @[[FROM_NATIVE_1]](ptr %0, <4 x i64> %1) {{.*}}{
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc <4 x i64> @[[TO_NATIVE_1]](ptr %0, <4 x i64> %1) {{.*}}{
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc <4 x i64> @[[TO_NATIVE_2]](ptr %0, i64 %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = insertelement <4 x i64> poison, i64 %{{[-a-zA-Z$._0-9]+}}, i64 0
; CHECK: %{{[-a-zA-Z$._0-9]+}} = shufflevector <4 x i64> %{{[-a-zA-Z$._0-9]+}}, <4 x i64> poison, <4 x i32> zeroinitializer
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}
