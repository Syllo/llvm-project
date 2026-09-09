; RUN: llc -mtriple=hexagon < %s | FileCheck %s
; RUN: llc -mtriple=hexagon -stop-after=finalize-isel < %s \
; RUN: | FileCheck %s --check-prefix=MIR
;
; CHECK-LABEL: foo_128_2x

; CHECK: v{{[0-9]+}} = vand(q0,r{{[0-9]+}})

; Check that the low and high packed words are returned in order.
; CHECK: r0 = vextract(v{{[0-9]+}},r{{[0-9]+}})
; CHECK: r1 = vextract(v{{[0-9]+}},r{{[0-9]+}})
; CHECK: jumpr r31

; MIR-LABEL: name: foo_128_2x
; MIR: [[LOW:%[0-9]+]]:intregs = V6_extractw [[LOW_REDUCTION:%[0-9]+]], [[WORD:%[0-9]+]]
; MIR: [[HIGH:%[0-9]+]]:intregs = V6_extractw [[HIGH_REDUCTION:%[0-9]+]], [[WORD]]
; MIR: [[PACKED:%[0-9]+]]:doubleregs = REG_SEQUENCE [[HIGH]], %subreg.isub_hi, [[LOW]], %subreg.isub_lo
; MIR: $d0 = COPY [[PACKED]]

define i64 @foo_128_2x(<64 x i16> %a0, <64 x i16> %a1) #0 {
  %v0 = icmp ugt <64 x i16> %a0, %a1
  %v1 = bitcast <64 x i1> %v0 to i64
  ret i64 %v1
}

;; The remaining test cases only check the bitvector compression stage, which is
;; the only distinction.

; CHECK-LABEL: foo_128_4x
; CHECK: v{{[0-9]+}} = vand(q0,r{{[0-9]+}})
define i32 @foo_128_4x(<32 x i16> %a0, <32 x i16> %a1) #0 {
  %v0 = icmp ugt <32 x i16> %a0, %a1
  %v1 = bitcast <32 x i1> %v0 to i32
  ret i32 %v1
}

; CHECK-LABEL: foo_64_2x
; CHECK: v{{[0-9]+}} = vand(q0,r{{[0-9]+}})
define i32 @foo_64_2x(<32 x i16> %a0, <32 x i16> %a1) #1 {
  %v0 = icmp ugt <32 x i16> %a0, %a1
  %v1 = bitcast <32 x i1> %v0 to i32
  ret i32 %v1
}

; CHECK-LABEL: foo_64_4x
; CHECK: v{{[0-9]+}} = vand(q0,r{{[0-9]+}})
define i16 @foo_64_4x(<16 x i16> %a0, <16 x i16> %a1) #1 {
  %v0 = icmp ugt <16 x i16> %a0, %a1
  %v1 = bitcast <16 x i1> %v0 to i16
  ret i16 %v1
}

attributes #0 = { nounwind readnone "target-cpu"="hexagonv68" "target-features"="+hvx,+hvx-length128b,-packets" }
attributes #1 = { nounwind readnone "target-cpu"="hexagonv68" "target-features"="+hvx,+hvx-length64b,-packets" }
