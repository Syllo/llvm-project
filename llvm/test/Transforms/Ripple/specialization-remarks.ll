; RUN: split-file %s %t
; RUN: not opt -passes=ripple -pass-remarks=ripple -pass-remarks-missed=ripple -disable-output %t/native.ll 2>&1 | FileCheck %s --check-prefix=NATIVE --implicit-check-not='LLVM ERROR'
; RUN: opt -passes=ripple -pass-remarks=ripple -verify-each -S %t/noreturn.ll 2>&1 | FileCheck %s --check-prefix=NORETURN

; Remarks must handle native operands even when promotion of the call fails.
; NATIVE: error: {{.*}}Ripple cannot create a vector type
; NATIVE: call not vectorized: no suitable vector implementation found for 'consume'
; NATIVE-SAME: (unchanged, Tensor[4]) -> Tensor[4]

; A noreturn specialization has no ReturnInst from which to read a shape.
; NORETURN: function 'sink' specialized for tensor operands
; NORETURN-SAME: -> unchanged}
; NORETURN: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.sink(
; NORETURN: unreachable

;--- native.ll
define void @caller(<2 x i64> %v, ptr %out) !dbg !3 {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %dst = getelementptr i64, ptr %out, i64 %id
  store i64 %id, ptr %dst
  call void @consume(<2 x i64> %v, i64 %id), !dbg !5
  ret void
}
declare void @consume(<2 x i64>, i64)
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!1}
!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !2, isOptimized: true, runtimeVersion: 0, emissionKind: LineTablesOnly)
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !DIFile(filename: "native.c", directory: "/")
!3 = distinct !DISubprogram(name: "caller", scope: !2, file: !2, line: 1, type: !4, spFlags: DISPFlagDefinition, unit: !0)
!4 = !DISubroutineType(types: !{})
!5 = !DILocation(line: 2, column: 1, scope: !3)

;--- noreturn.ll
define void @caller(ptr %out) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  call void @sink(ptr %out, i64 %id)
  unreachable
}
define internal void @sink(ptr %out, i64 %x) noinline noreturn {
  %dst = getelementptr i64, ptr %out, i64 %x
  store i64 %x, ptr %dst
  unreachable
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
