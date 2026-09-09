; RUN: llc -mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b \
; RUN:   -stop-after=finalize-isel < %s | FileCheck %s

define void @store_v32i1_align4(<32 x i1> %q, ptr %dst) {
; CHECK-LABEL: name:            store_v32i1_align4
; CHECK: S2_storeri_io {{.*}} :: (volatile store (s32) into %ir.dst)
  store volatile <32 x i1> %q, ptr %dst, align 4
  ret void
}

define void @store_v32i1_align8(<32 x i1> %q, ptr %dst) {
; CHECK-LABEL: name:            store_v32i1_align8
; CHECK: S2_storeri_io {{.*}} :: (volatile non-temporal store (s32) into %ir.dst, align 8)
  store volatile <32 x i1> %q, ptr %dst, align 8, !nontemporal !0
  ret void
}

define void @store_v64i1_align8(<64 x i1> %q, ptr %dst) {
; CHECK-LABEL: name:            store_v64i1_align8
; CHECK: S2_storerd_io {{.*}} :: (volatile non-temporal store (s64) into %ir.dst)
  store volatile <64 x i1> %q, ptr %dst, align 8, !nontemporal !0
  ret void
}

!0 = !{i32 1}
