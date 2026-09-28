; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not=llvm.masked.load --implicit-check-not=llvm.masked.store

; The helper copies a native vector as a uniform value. Its memory operations
; use a scalar guard, even though their LLVM value type is a vector.
define void @masked_native_copy(ptr %out, ptr %in, i64 %limit) {
; CHECK-LABEL: define void @masked_native_copy(
; CHECK: call fastcc void @[[COPY:masked.ripple.specialization.final.[0-9]+.copy]](ptr poison, ptr %out, ptr %in, <4 x i1> %{{.*}})
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %active = icmp ult i64 %id, %limit
  br i1 %active, label %then, label %end
then:
  call void @copy(ptr %bs, ptr %out, ptr %in)
  br label %end
end:
  ret void
}

define internal void @copy(ptr %bs, ptr %out, ptr %in) noinline {
  %v = load <2 x i64>, ptr %in, align 8
  %n = call i64 @llvm.ripple.block.getsize.i64(ptr %bs, i64 0)
  %r = insertelement <2 x i64> %v, i64 %n, i32 0
  store <2 x i64> %r, ptr %out, align 8
  ret void
}

; CHECK: define internal fastcc void @[[COPY]](ptr %0, ptr %1, ptr %2, <4 x i1> %3)
; CHECK: [[BITS:%.*]] = bitcast <4 x i1> %3 to i4
; CHECK: [[NONE:%.*]] = icmp eq i4 [[BITS]], 0
; CHECK: br i1 [[NONE]],
; CHECK: load <2 x i64>, ptr %2
; CHECK: br i1 [[NONE]],
; CHECK: insertelement <2 x i64> %{{.*}}, i64 4,
; CHECK: store <2 x i64> %{{.*}}, ptr %1

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.getsize.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
