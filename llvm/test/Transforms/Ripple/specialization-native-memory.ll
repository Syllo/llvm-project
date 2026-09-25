; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s

; A varying scalar argument triggers specialization. Native-vector loads and
; stores in the callee are uniform and must be guarded as whole operations.
; Native masked operations combine their predicate with a splat of the scalar
; Ripple mask.
define void @caller(ptr %out, ptr %in, ptr %lanes, i64 %limit) {
; CHECK-LABEL: define void @caller(
; CHECK: call fastcc void @[[COPY:masked.ripple.specialization.final.[0-9]+.copy]](
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %active = icmp ult i64 %id, %limit
  br i1 %active, label %then, label %end
then:
  call void @copy(ptr %out, ptr %in, ptr %lanes, i64 %id)
  br label %end
end:
  ret void
}

define internal void @copy(ptr %out, ptr %in, ptr %lanes, i64 %lane) noinline {
  %v = load <2 x i64>, ptr %in, align 8
  %r = add <2 x i64> %v, <i64 1, i64 9>
  store <2 x i64> %r, ptr %out, align 8
  %masked = call <2 x i64> @llvm.masked.load.v2i64.p0(ptr %in, i32 8, <2 x i1> <i1 true, i1 false>, <2 x i64> poison)
  call void @llvm.masked.store.v2i64.p0(<2 x i64> %masked, ptr %out, i32 8, <2 x i1> <i1 true, i1 false>)
  %dst = getelementptr i64, ptr %lanes, i64 %lane
  store i64 %lane, ptr %dst
  ret void
}

; CHECK: define internal fastcc void @[[COPY]](ptr %0, ptr %1, ptr %2, <4 x i64> %3, <4 x i1> %4)
; CHECK: [[BITS:%.*]] = bitcast <4 x i1> %4 to i4
; CHECK: [[ACTIVE:%.*]] = icmp {{(eq|ne)}} i4 [[BITS]], 0
; CHECK: br i1 [[ACTIVE]],
; CHECK: load <2 x i64>, ptr %1
; CHECK: br i1 [[ACTIVE]],
; CHECK: store <2 x i64> %{{.*}}, ptr %0
; CHECK: [[MASKSPLATINSERT:%.*]] = insertelement <2 x i1> poison, i1 [[ACTIVE]], i64 0
; CHECK: [[MASKSPLAT:%.*]] = shufflevector <2 x i1> [[MASKSPLATINSERT]], <2 x i1> poison, <2 x i32> zeroinitializer
; CHECK: [[MASKEDLOADMASK:%.*]] = and <2 x i1> [[MASKSPLAT]], <i1 true, i1 false>
; CHECK: call <2 x i64> @llvm.masked.load.v2i64.p0(ptr{{( align 8)?}} %1, <2 x i1> [[MASKEDLOADMASK]], <2 x i64> poison)
; CHECK: [[MASKEDSPLATINSERT:%.*]] = insertelement <2 x i1> poison, i1 [[ACTIVE]], i64 0
; CHECK: [[MASKEDSPLAT:%.*]] = shufflevector <2 x i1> [[MASKEDSPLATINSERT]], <2 x i1> poison, <2 x i32> zeroinitializer
; CHECK: [[MASKEDSTOREMASK:%.*]] = and <2 x i1> [[MASKEDSPLAT]], <i1 true, i1 false>
; CHECK: call void @llvm.masked.store.v2i64.p0(<2 x i64> %{{.*}}, ptr{{( align 8)?}} %0, <2 x i1> [[MASKEDSTOREMASK]])

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
declare <2 x i64> @llvm.masked.load.v2i64.p0(ptr, i32 immarg, <2 x i1>, <2 x i64>)
declare void @llvm.masked.store.v2i64.p0(<2 x i64>, ptr, i32 immarg, <2 x i1>)
