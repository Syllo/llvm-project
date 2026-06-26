; RUN: opt -passes='module(function(mem2reg),ripple,function(dce))' -S < %s | FileCheck %s --implicit-check-not="warning:"
source_filename = "another_shape_prop_bug.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable
define dso_local void @foo(ptr nocapture noundef readonly %A, ptr nocapture noundef writeonly %B) local_unnamed_addr #0 {
entry:
  %A_tile = alloca [10 x [10 x float]], align 16
  %BS = tail call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 10, i64 10, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @llvm.lifetime.start.p0(i64 400, ptr nonnull %A_tile) #4
  %0 = tail call i64 @llvm.ripple.block.index.i64(ptr %BS, i64 0)
  %1 = tail call i64 @llvm.ripple.block.index.i64(ptr %BS, i64 1)
  %arrayidx1 = getelementptr inbounds [10 x float], ptr %A, i64 %1, i64 %0
  ; CHECK: load <100 x float>
  %2 = load float, ptr %arrayidx1, align 4, !tbaa !5
  %arrayidx3 = getelementptr inbounds [10 x [10 x float]], ptr %A_tile, i64 0, i64 %1, i64 %0
  store float %2, ptr %arrayidx3, align 4, !tbaa !5
  %arrayidx5 = getelementptr inbounds [10 x [10 x float]], ptr %A_tile, i64 0, i64 %0, i64 %1
  ; CHECK-NOT: @llvm.masked.gather
  ; The transpose load through A_tile now fits the bounded-window load+shuffle
  ; path (byte stride 40 over a 100-element tile fits in a single contiguous
  ; load); after A_tile is DCE'd the transpose materializes directly as a
  ; shufflevector over the load of A.
  ; CHECK: shufflevector <100 x float> %{{[^,]+}}, <100 x float> poison, <100 x i32> <i32 0, i32 10, i32 20, i32 30, i32 40, i32 50, i32 60, i32 70, i32 80, i32 90, i32 1, i32 11, i32 21, i32 31, i32 41, i32 51, i32 61, i32 71, i32 81, i32 91, i32 2, i32 12, i32 22, i32 32, i32 42, i32 52, i32 62, i32 72, i32 82, i32 92, i32 3, i32 13, i32 23, i32 33, i32 43, i32 53, i32 63, i32 73, i32 83, i32 93, i32 4, i32 14, i32 24, i32 34, i32 44, i32 54, i32 64, i32 74, i32 84, i32 94, i32 5, i32 15, i32 25, i32 35, i32 45, i32 55, i32 65, i32 75, i32 85, i32 95, i32 6, i32 16, i32 26, i32 36, i32 46, i32 56, i32 66, i32 76, i32 86, i32 96, i32 7, i32 17, i32 27, i32 37, i32 47, i32 57, i32 67, i32 77, i32 87, i32 97, i32 8, i32 18, i32 28, i32 38, i32 48, i32 58, i32 68, i32 78, i32 88, i32 98, i32 9, i32 19, i32 29, i32 39, i32 49, i32 59, i32 69, i32 79, i32 89, i32 99>
  %3 = load float, ptr %arrayidx5, align 4, !tbaa !5
  %arrayidx7 = getelementptr inbounds [10 x float], ptr %B, i64 %0, i64 %1
  ; CHECK: call void @llvm.masked.scatter.v100f32.v100p0
  store float %3, ptr %arrayidx7, align 4, !tbaa !5
  call void @llvm.lifetime.end.p0(i64 400, ptr nonnull %A_tile) #4
  ret void
}

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64) #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg) #3

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #2

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { mustprogress nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: read) }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Clang $LLVM_VERSION_MAJOR.$LLVM_VERSION_MINOR"}
!5 = !{!6, !6, i64 0}
!6 = !{!"float", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
