; RUN: opt -passes="ripple" -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-IC
; RUN: opt -ripple-run-only-ripple-passes -passes="ripple" -S %s | FileCheck %s --check-prefixes=CHECK,CHECK-NOIC
; RUN: opt -passes="ripple" -S -pass-remarks-missed=ripple %s 2>&1 >/dev/null | FileCheck %s --check-prefix=REMARK

target datalayout = "e-m:e-p:32:32-i64:64-a:0-n32-S128"
target triple = "hexagon-unknown-elf"

; CHECK: define internal void @ripple_vector_candidate_alwaysinline(
define internal void @ripple_vector_candidate_alwaysinline(ptr %p) #0 {
entry:
  store i32 42, ptr %p, align 4
  ret void
}

; CHECK: define internal void @ripple_vector_candidate_normal(
define internal void @ripple_vector_candidate_normal(ptr %p) #1 {
entry:
  %p1 = getelementptr inbounds i32, ptr %p, i32 1
  store i32 43, ptr %p1, align 4
  ret void
}

; CHECK: define internal void @ripple_vector_candidate_noinline(
define internal void @ripple_vector_candidate_noinline(ptr %p) #2 {
entry:
  %p2 = getelementptr inbounds i32, ptr %p, i32 2
  store i32 44, ptr %p2, align 4
  ret void
}

define void @caller(ptr %p) {
; CHECK-LABEL: @caller(
; CHECK-NOT: call void @ripple_vector_candidate_alwaysinline
; CHECK-NOT: call void @ripple_vector_candidate_normal
; CHECK-IC: store i32 42, ptr %p, align 4
; CHECK-IC: store i32 43, ptr {{%.*}}, align 4
; CHECK-NOIC: store i32 42, ptr %p, align 4
; CHECK-NOIC: store i32 43, ptr {{%.*}}, align 4
; CHECK: call void @ripple_vector_candidate_noinline(ptr{{.*}} %p)
; The noinline candidate is declined by the cost model, which emits a
; ripple-namespaced missed remark including the cost model's reason.
; REMARK: remark: {{.*}} did not inline ripple-vector-candidate 'ripple_vector_candidate_noinline' into 'caller': noinline function attribute
entry:
  call void @ripple_vector_candidate_alwaysinline(ptr %p)
  call void @ripple_vector_candidate_normal(ptr %p)
  call void @ripple_vector_candidate_noinline(ptr %p)
  ret void
}

attributes #0 = { alwaysinline "ripple-vector-candidate" }
attributes #1 = { "ripple-vector-candidate" }
attributes #2 = { noinline "ripple-vector-candidate" }
