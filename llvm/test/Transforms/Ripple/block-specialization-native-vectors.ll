; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s

; Native vectors remain uniform when a block descriptor requests a
; specialization. They must not become vectors of native vectors.
define <2 x i64> @native_argument(<2 x i64> %v) {
; CHECK-LABEL: define <2 x i64> @native_argument(<2 x i64> %v) {
; CHECK: call fastcc <2 x i64> @[[INSERT:ripple.specialization.final.[0-9]+.insert_block_size]](ptr poison, <2 x i64> %v)
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call <2 x i64> @insert_block_size(ptr %bs, <2 x i64> %v)
  ret <2 x i64> %r
}

; The constant reuses the same native-vector specialization.
define <2 x i64> @native_constant() {
; CHECK-LABEL: define <2 x i64> @native_constant() {
; CHECK: call fastcc <2 x i64> @[[INSERT]](ptr poison, <2 x i64> <i64 7, i64 9>)
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %r = call <2 x i64> @insert_block_size(ptr %bs, <2 x i64> <i64 7, i64 9>)
  ret <2 x i64> %r
}

define internal <2 x i64> @insert_block_size(ptr %bs, <2 x i64> %v) noinline {
  %n = call i64 @llvm.ripple.block.getsize.i64(ptr %bs, i64 0)
  %r = insertelement <2 x i64> %v, i64 %n, i32 0
  ret <2 x i64> %r
}

; A uniform native-vector input can feed a specialization with a varying
; scalar result.
define void @native_to_tensor(<2 x i64> %v, ptr %out) {
; CHECK-LABEL: define void @native_to_tensor(<2 x i64> %v, ptr %out) {
; CHECK: call fastcc <4 x i64> @[[SEED:ripple.specialization.final.[0-9]+.native_seed]](ptr poison, <2 x i64> %v)
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %x = call i64 @native_seed(ptr %bs, <2 x i64> %v)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %x, ptr %dst
  ret void
}

define internal i64 @native_seed(ptr %bs, <2 x i64> %v) noinline {
  %seed = extractelement <2 x i64> %v, i32 0
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %x = add i64 %seed, %id
  ret i64 %x
}

declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.getsize.i64(ptr, i64 immarg)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
