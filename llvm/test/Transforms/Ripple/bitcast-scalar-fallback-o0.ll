; RUN: opt -passes=ripple -ripple-run-only-ripple-passes -verify-each -S %s | FileCheck %s --implicit-check-not='call {{.*}}@llvm.ripple.bitcast'

; Clang -O0 stores block arguments in allocas. Single-lane cleanup must trace
; the load back to the unresolved argument and retain the scalar fallback.
define <4 x float> @round_trip(ptr %block, <4 x float> %v) {
; CHECK-LABEL: define <4 x float> @round_trip(
; CHECK: [[X:%.*]] = extractelement <4 x float> %v, i64 0
; CHECK: [[INSERT:%.*]] = insertelement <4 x float> poison, float [[X]], i64 0
; CHECK: [[RESULT:%.*]] = shufflevector <4 x float> [[INSERT]], <4 x float> poison, <4 x i32> zeroinitializer
; CHECK: ret <4 x float> [[RESULT]]
  %block.addr = alloca ptr, align 8
  store ptr %block, ptr %block.addr, align 8
  %loaded.block = load ptr, ptr %block.addr, align 8
  %x = call float @llvm.ripple.from.fixed.vector.v4f32(ptr %loaded.block, <4 x float> %v)
  %result = call <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float %x)
  ret <4 x float> %result
}

; The scalar operand of to.vec has the same unresolved-argument fallback,
; including when Clang -O0 materializes the argument through an alloca.
define <4 x float> @to_native(float %x) {
; CHECK-LABEL: define <4 x float> @to_native(
; CHECK: [[LOADED:%.*]] = load float, ptr %x.addr
; CHECK: [[INSERT:%.*]] = insertelement <4 x float> poison, float [[LOADED]], i64 0
; CHECK: [[RESULT:%.*]] = shufflevector <4 x float> [[INSERT]], <4 x float> poison, <4 x i32> zeroinitializer
; CHECK: ret <4 x float> [[RESULT]]
  %x.addr = alloca float, align 4
  store float %x, ptr %x.addr, align 4
  %loaded.x = load float, ptr %x.addr, align 4
  %result = call <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float %loaded.x)
  ret <4 x float> %result
}

declare float @llvm.ripple.from.fixed.vector.v4f32(ptr, <4 x float>)
declare <4 x float> @llvm.ripple.to.fixed.vector.v4f32(float)
