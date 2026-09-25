; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s

; Block descriptors must participate in specialization even when data shapes
; are unchanged. In particular the two varying calls must add 2 and 4.
define void @different_blocks(ptr %in, ptr %out2, ptr %out4) {
; CHECK-LABEL: define void @different_blocks(ptr %in, ptr %out2, ptr %out4) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = load <4 x i64>, ptr %{{[-a-zA-Z$._0-9]+}}, align 4
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[ADD_BLOCK_SIZE_1:ripple.specialization.final.[0-9]+.add_block_size]](ptr poison, <4 x i64> %{{[-a-zA-Z$._0-9]+}})
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc <4 x i64> @[[ADD_BLOCK_SIZE_2:ripple.specialization.final.[0-9]+.add_block_size]](ptr poison, <4 x i64> %{{[-a-zA-Z$._0-9]+}})
; CHECK: store <4 x i64> %{{[-a-zA-Z$._0-9]+}}, ptr %{{[-a-zA-Z$._0-9]+}}, align 4
; CHECK: store <4 x i64> %{{[-a-zA-Z$._0-9]+}}, ptr %{{[-a-zA-Z$._0-9]+}}, align 4
; CHECK: ret void
  %bs2 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %bs4 = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs4, i64 0)
  %src = getelementptr i64, ptr %in, i64 %id
  %x = load i64, ptr %src
  %a = call i64 @add_block_size(ptr %bs2, i64 %x)
  %b = call i64 @add_block_size(ptr %bs4, i64 %x)
  %dst2 = getelementptr i64, ptr %out2, i64 %id
  %dst4 = getelementptr i64, ptr %out4, i64 %id
  store i64 %a, ptr %dst2
  store i64 %b, ptr %dst4
  ret void
}
define internal i64 @add_block_size(ptr %bs, i64 %x) noinline {
  %size = call i64 @llvm.ripple.block.getsize.i64(ptr %bs, i64 0)
  %r = add i64 %x, %size
  ret i64 %r
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.getsize.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)

define i64 @uniform_two(i64 %x) {
; CHECK-LABEL: define i64 @uniform_two(i64 %x) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc i64 @[[ADD_BLOCK_SIZE_3:ripple.specialization.final.[0-9]+.add_block_size]](ptr poison, i64 %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 2, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call i64 @add_block_size(ptr %bs, i64 %x)
  ret i64 %r
}

define i64 @uniform_four(i64 %x) {
; CHECK-LABEL: define i64 @uniform_four(i64 %x) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc i64 @[[ADD_BLOCK_SIZE_4:ripple.specialization.final.[0-9]+.add_block_size]](ptr poison, i64 %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call i64 @add_block_size(ptr %bs, i64 %x)
  ret i64 %r
}

define i64 @equivalent_four(i64 %x) {
; CHECK-LABEL: define i64 @equivalent_four(i64 %x) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc i64 @[[ADD_BLOCK_SIZE_4]](ptr poison, i64 %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %r = call i64 @add_block_size(ptr %bs, i64 %x)
  ret i64 %r
}

; A rank-zero caller still requests an exact scalar block specialization.
define i64 @single_lane(i64 %x) {
; CHECK-LABEL: define i64 @single_lane(i64 %x) {
; CHECK-NEXT: [[R:%.*]] = call fastcc i64 @[[ADD_BLOCK_SIZE_SCALAR:ripple.specialization.final.[0-9]+.add_block_size]](ptr poison, i64 %x)
; CHECK-NEXT: ret i64 [[R]]
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call i64 @add_block_size(ptr %bs, i64 %x)
  ret i64 %r
}

; All data arguments are uniform; BS supplies the four-lane mask context.
define void @masked_fill(ptr %out, i64 %x, i64 %limit) {
; CHECK-LABEL: define void @masked_fill(ptr %out, i64 %x, i64 %limit) {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = insertelement <4 x i64> poison, i64 %{{[-a-zA-Z$._0-9]+}}, i64 0
; CHECK: %{{[-a-zA-Z$._0-9]+}} = shufflevector <4 x i64> %{{[-a-zA-Z$._0-9]+}}, <4 x i64> poison, <4 x i32> zeroinitializer
; CHECK: %{{[-a-zA-Z$._0-9]+}} = icmp ugt <4 x i64> %{{[-a-zA-Z$._0-9]+}}, <i64 0, i64 1, i64 2, i64 3>
; CHECK: ret void
; CHECK: call fastcc void @[[FILL_1:masked.ripple.specialization.final.[0-9]+.fill]](ptr poison, ptr %{{[-a-zA-Z$._0-9]+}}, i64 %{{[-a-zA-Z$._0-9]+}}, <4 x i1> %{{[-a-zA-Z$._0-9]+}})
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %active = icmp ult i64 %id, %limit
  br i1 %active, label %then, label %end
then:
  call void @fill(ptr %bs, ptr %out, i64 %x)
  br label %end
end:
  ret void
}

define internal void @fill(ptr %bs, ptr %out, i64 %x) noinline {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %x, ptr %dst
  ret void
}

define void @single_lane_fill(ptr %out, i64 %x) {
; CHECK-LABEL: define void @single_lane_fill(ptr %out, i64 %x) {
; CHECK-NEXT: call fastcc void @[[FILL_SCALAR:ripple.specialization.final.[0-9]+.fill]](ptr poison, ptr %out, i64 %x)
; CHECK-NEXT: ret void
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @fill(ptr %bs, ptr %out, i64 %x)
  ret void
}

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)

; Keys include every block argument and its dimension extents, not just width.
define i64 @two_block_arguments() {
; CHECK-LABEL: define i64 @two_block_arguments() {
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc i64 @[[SIZES_1:ripple.specialization.final.[0-9]+.sizes]](ptr poison, ptr poison)
; CHECK: %{{[-a-zA-Z$._0-9]+}} = call fastcc i64 @[[SIZES_2:ripple.specialization.final.[0-9]+.sizes]](ptr poison, ptr poison)
; CHECK: %{{[-a-zA-Z$._0-9]+}} = sub i64 %{{[-a-zA-Z$._0-9]+}}, %{{[-a-zA-Z$._0-9]+}}
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}
  %rows = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %cols = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 1, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %a = call i64 @sizes(ptr %rows, ptr %cols)
  %b = call i64 @sizes(ptr %cols, ptr %rows)
  %r = sub i64 %a, %b
  ret i64 %r
}

define internal i64 @sizes(ptr %a, ptr %b) noinline {
  %x = call i64 @llvm.ripple.block.getsize.i64(ptr %a, i64 0)
  %y = call i64 @llvm.ripple.block.getsize.i64(ptr %b, i64 1)
  %r = add i64 %x, %y
  ret i64 %r
}

; CHECK: define internal fastcc i64 @[[SIZES_1]](ptr %0, ptr %1) {{.*}}{
; CHECK: ret i64 8

; CHECK: define internal fastcc i64 @[[SIZES_2]](ptr %0, ptr %1) {{.*}}{
; CHECK: ret i64 2

; CHECK: define internal fastcc void @[[FILL_SCALAR]](ptr %0, ptr %1, i64 %2) {{.*}}{
; CHECK: store i64 %2, ptr %1, align 4
; CHECK: ret void

; CHECK: define internal fastcc void @[[FILL_1]](ptr %0, ptr %1, i64 %2, <4 x i1> %3) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = insertelement <4 x i64> poison, i64 %{{[-a-zA-Z$._0-9]+}}, i64 0
; CHECK: %{{[-a-zA-Z$._0-9]+}} = shufflevector <4 x i64> %{{[-a-zA-Z$._0-9]+}}, <4 x i64> poison, <4 x i32> zeroinitializer
; CHECK: call void @llvm.masked.store.v4i64.p0(<4 x i64> %{{[-a-zA-Z$._0-9]+}}, ptr align 4 %{{[-a-zA-Z$._0-9]+}}, <4 x i1> %{{[-a-zA-Z$._0-9]+}})
; CHECK: ret void

; CHECK: define internal fastcc i64 @[[ADD_BLOCK_SIZE_SCALAR]](ptr %0, i64 %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = add i64 %{{[-a-zA-Z$._0-9]+}}, 1
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc i64 @[[ADD_BLOCK_SIZE_4]](ptr %0, i64 %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = add i64 %{{[-a-zA-Z$._0-9]+}}, 4
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc i64 @[[ADD_BLOCK_SIZE_3]](ptr %0, i64 %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = add i64 %{{[-a-zA-Z$._0-9]+}}, 2
; CHECK: ret i64 %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc <4 x i64> @[[ADD_BLOCK_SIZE_1]](ptr %0, <4 x i64> %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = add <4 x i64> %{{[-a-zA-Z$._0-9]+}}, splat (i64 2)
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}

; CHECK: define internal fastcc <4 x i64> @[[ADD_BLOCK_SIZE_2]](ptr %0, <4 x i64> %1) {{.*}}{
; CHECK: %{{[-a-zA-Z$._0-9]+}} = add <4 x i64> %{{[-a-zA-Z$._0-9]+}}, splat (i64 4)
; CHECK: ret <4 x i64> %{{[-a-zA-Z$._0-9]+}}
