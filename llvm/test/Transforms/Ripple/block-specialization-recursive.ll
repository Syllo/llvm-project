; RUN: opt -passes=ripple -verify-each -S %s | FileCheck %s --check-prefixes=CHECK,ORIGINAL --implicit-check-not='call {{.*}}@llvm.ripple'
; RUN: opt -passes='ripple,globaldce' -verify-each -S %s | FileCheck %s --implicit-check-not='define internal void @self_helper' --implicit-check-not='define internal void @mutual_a' --implicit-check-not='define internal void @mutual_b'

; Original helpers have no active Ripple dimensions, so their unresolved block
; parameters use single-lane cleanup. Leave removal of unused cycles to GlobalDCE.
; CHECK-DAG: define void @recursive_caller(
; CHECK-DAG: define void @mutual_caller(
; ORIGINAL-DAG: define internal void @self_helper(
; ORIGINAL-DAG: define internal void @mutual_a(
; ORIGINAL-DAG: define internal void @mutual_b(
; CHECK-DAG: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.self_helper(
; CHECK-DAG: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.mutual_a(
; CHECK-DAG: define internal fastcc void @ripple.specialization.final.{{[0-9]+}}.mutual_b(
; CHECK-DAG: define void @live_root(
; CHECK-DAG: define internal void @live_helper(
; CHECK-DAG: define internal void @address_taken(
; ORIGINAL-DAG: define internal void @callee_and_argument(

@callback = global ptr @address_taken

define void @recursive_caller(ptr %out, i64 %n) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @self_helper(ptr %bs, ptr %out, i64 %n)
  ret void
}
define internal void @self_helper(ptr %bs, ptr %out, i64 %n) noinline {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %p = getelementptr i64, ptr %out, i64 %id
  store i64 %n, ptr %p
  %c = icmp eq i64 %n, 0
  br i1 %c, label %done, label %recurse
recurse:
  %next = sub i64 %n, 1
  call void @self_helper(ptr %bs, ptr %out, i64 %next)
  br label %done
done:
  ret void
}

define void @mutual_caller(ptr %out, i64 %n) {
  %bs = call ptr @llvm.ripple.block.setshape.i64(i64 0, i64 4, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1, i64 1)
  call void @mutual_a(ptr %bs, ptr %out, i64 %n)
  ret void
}
define internal void @mutual_a(ptr %bs, ptr %out, i64 %n) noinline {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %p = getelementptr i64, ptr %out, i64 %id
  store i64 %n, ptr %p
  %c = icmp eq i64 %n, 0
  br i1 %c, label %done, label %recurse
recurse:
  %next = sub i64 %n, 1
  call void @mutual_b(ptr %bs, ptr %out, i64 %next)
  br label %done
done:
  ret void
}
define internal void @mutual_b(ptr %bs, ptr %out, i64 %n) noinline {
  %id = call i64 @llvm.ripple.block.index.i64(ptr %bs, i64 0)
  %p = getelementptr i64, ptr %out, i64 %id
  store i64 %n, ptr %p
  %c = icmp eq i64 %n, 0
  br i1 %c, label %done, label %recurse
recurse:
  %next = sub i64 %n, 1
  call void @mutual_a(ptr %bs, ptr %out, i64 %next)
  br label %done
done:
  ret void
}

; Keep originals that are externally reachable or whose addresses escape.
define void @live_root(ptr %out) {
  call void @live_helper(ptr %out)
  ret void
}
define internal void @live_helper(ptr %out) noinline {
  store i64 42, ptr %out
  ret void
}
define internal void @address_taken(ptr %out) {
  store i64 7, ptr %out
  ret void
}
; A use as an argument is an escape even when the callee is the same function.
define internal void @callee_and_argument(ptr %callback) {
  call void @callee_and_argument(ptr @callee_and_argument)
  ret void
}
declare ptr @llvm.ripple.block.setshape.i64(i64 immarg, i64, i64, i64, i64, i64, i64, i64, i64, i64, i64)
declare i64 @llvm.ripple.block.index.i64(ptr, i64 immarg)
