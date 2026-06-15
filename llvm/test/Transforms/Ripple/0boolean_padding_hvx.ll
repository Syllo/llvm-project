; REQUIRES: hexagon-registered-target
; RUN: opt -S -passes="module(ripple)" -ripple-pad-to-target-simd %s -o - | FileCheck %s
target triple = "hexagon"

; The boolean (i1) vectors derived from the i32 comparison/predicate path must
; be padded to a single HVX predicate-register width. For i32-lane data on a
; 128-byte HVX target that is <64 x i1> -- NOT the over-wide <1024 x i1> that a
; data-register-calibrated getMinimumVF would pick, and NOT the <128 x i1>
; byte-mask / predicate-pair width (which legitimately only shows up as the
; mask of the i8 store below).

; CHECK-LABEL: define dso_local void @foo
define dso_local void @foo(ptr noundef %a, ptr noundef %b, ptr noundef %result) #0 {
entry:
  %0 = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 33, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %1 = call i32 @llvm.ripple.block.index.i32(ptr %0, i32 0)
  %arrayidx = getelementptr inbounds nuw i32, ptr %a, i32 %1
  %2 = load i32, ptr %arrayidx, align 4
  %arrayidx1 = getelementptr inbounds nuw i32, ptr %b, i32 %1
  %3 = load i32, ptr %arrayidx1, align 4
  %cmp = icmp sgt i32 %2, %3
  br i1 %cmp, label %land.rhs, label %land.end

land.rhs:                                         ; preds = %entry
  %arrayidx2 = getelementptr inbounds nuw i32, ptr %a, i32 %1
  %4 = load i32, ptr %arrayidx2, align 4
  %tobool = icmp ne i32 %4, 0
  br label %land.end

land.end:                                         ; preds = %land.rhs, %entry
  %5 = phi i1 [ false, %entry ], [ %tobool, %land.rhs ]
  %arrayidx3 = getelementptr inbounds nuw i8, ptr %result, i32 %1
  %storedv = zext i1 %5 to i8
  store i8 %storedv, ptr %arrayidx3, align 1
  ret void
}

; CHECK: %[[CMP:[a-zA-Z0-9._]+]] = icmp sgt <64 x i32>
; CHECK: icmp ne <64 x i32>
; CHECK: select <64 x i1> %[[CMP]], <64 x i1> %{{[a-zA-Z0-9._]+}}, <64 x i1> zeroinitializer
; CHECK: shufflevector <64 x i1> %{{[a-zA-Z0-9._]+}}, <64 x i1> poison, <128 x i32> <i32 0,
; CHECK-SAME: i32 poison>
; CHECK: and <64 x i1> %[[CMP]],

attributes #0 = { nounwind "target-features"="+hvx-length128b,+hvxv79,+v79" }
