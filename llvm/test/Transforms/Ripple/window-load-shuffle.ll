; RUN: opt -passes=ripple,dce -S %s | FileCheck  %s --implicit-check-not="warning:"


declare <8 x float> @llvm.masked.gather.v8f32.v8p0(<8 x ptr>, <8 x i1>, <8 x float>)

define void @window_load_shuffle(ptr readonly %A, ptr writeonly %B) {
; CHECK-LABEL: define void @window_load_shuffle
; CHECK-NOT: llvm.masked.gather
; CHECK: [[BASE:%.*]] = getelementptr i8, ptr {{%.*}}, i64 80
; CHECK: [[LD:%.*]] = load <8 x float>, ptr [[BASE]]
; CHECK: [[SHUF:%.*]] = shufflevector <8 x float> [[LD]], <8 x float> poison, <8 x i32> <i32 3, i32 5, i32 0, i32 1, i32 1, i32 7, i32 5, i32 4>
; CHECK: store <8 x float> [[SHUF]]
entry:
  %BS = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 8, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %v0 = call i64 @llvm.ripple.block.index.i64(ptr %BS, i64 0)

  ; Build idx(v0):
  ; v0: 0  1  2  3  4  5  6  7
  ; idx:23 25 20 21 21 27 25 24

  %is0 = icmp eq i64 %v0, 0
  %is1 = icmp eq i64 %v0, 1
  %is2 = icmp eq i64 %v0, 2
  %is3 = icmp eq i64 %v0, 3
  %is4 = icmp eq i64 %v0, 4
  %is5 = icmp eq i64 %v0, 5
  %is6 = icmp eq i64 %v0, 6

  %idx6 = select i1 %is6, i64 25, i64 24
  %idx5 = select i1 %is5, i64 27, i64 %idx6
  %idx4 = select i1 %is4, i64 21, i64 %idx5
  %idx3 = select i1 %is3, i64 21, i64 %idx4
  %idx2 = select i1 %is2, i64 20, i64 %idx3
  %idx1 = select i1 %is1, i64 25, i64 %idx2
  %idx  = select i1 %is0, i64 23, i64 %idx1

  %ptr = getelementptr float, ptr %A, i64 %idx
  %val = load float, ptr %ptr, align 4

  %out = getelementptr float, ptr %B, i64 %v0
  store float %val, ptr %out, align 4
  ret void
}
