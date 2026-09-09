; RUN: llc --mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length64b < %s | FileCheck %s
; RUN: llc --mtriple=hexagon -mcpu=hexagonv79 -mattr=+hvx-length128b < %s | FileCheck %s

define i32 @pack_v32i1(<32 x i1> %m) {
; CHECK-LABEL: pack_v32i1:
; CHECK-NOT: setbit(
; CHECK: vor(
; CHECK: vextract(
  %y = bitcast <32 x i1> %m to i32
  ret i32 %y
}

define i64 @pack_v64i1(<64 x i1> %m) {
; CHECK-LABEL: pack_v64i1:
; CHECK-NOT: setbit(
; CHECK: vor(
; CHECK: vextract(
; CHECK: vextract(
  %y = bitcast <64 x i1> %m to i64
  ret i64 %y
}
