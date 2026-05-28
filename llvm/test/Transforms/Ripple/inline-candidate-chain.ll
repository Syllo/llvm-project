; RUN: opt -passes="ripple" -S %s | FileCheck %s
; RUN: opt -ripple-run-only-ripple-passes -passes="ripple" -S %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-a:0-n32-S128"
target triple = "hexagon-unknown-elf"

; CHECK: define internal void @ripple_vector_candidate_chain_b(
define internal void @ripple_vector_candidate_chain_b(ptr %p) #0 {
entry:
  store volatile i32 99, ptr %p, align 4
  ret void
}

; CHECK: define internal void @ripple_vector_candidate_chain_a(
define internal void @ripple_vector_candidate_chain_a(ptr %p) #0 {
entry:
  call void @ripple_vector_candidate_chain_b(ptr %p)
  ret void
}

define void @caller_chain(ptr %p) {
; CHECK-LABEL: @caller_chain(
; CHECK-NOT: call void @ripple_vector_candidate_chain_a
; CHECK-NOT: call void @ripple_vector_candidate_chain_b
; CHECK: store volatile i32 99, ptr %p, align 4
entry:
  call void @ripple_vector_candidate_chain_a(ptr %p)
  ret void
}

attributes #0 = { "ripple-vector-candidate" }
