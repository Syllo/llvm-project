; RUN: llc -O2 -mtriple=hexagon -mcpu=hexagonv75 \
; RUN: -mattr=+hvxv75,+hvx-length128b < %s | FileCheck %s

; Verify the callee packs the predicate via the store path.
; CHECK-LABEL: cmp32:
; CHECK-NOT: vrmpy
; CHECK: v{{[0-9]+}} = vand(q{{[0-9]+}},r{{[0-9]+}})
; CHECK-COUNT-5: v{{[0-9]+}} = vor(
; CHECK: r{{[0-9]+}} = vextract(
; CHECK: jumpr r31

; Verify the caller rebuilds the predicate using the
; shift/sign-extension form rather than the old one-hot form.

; CHECK-LABEL: relu32:
; CHECK: [[W:v([0-9]+)]] = vsplat(r{{[0-9]+}})
; CHECK: [[SHL:v([0-9]+)]].w = vasl([[W]].w,v{{[0-9]+}}.w)
; CHECK: [[SRA:v([0-9]+)]].w = vasr([[SHL]].w,r{{[0-9]+}})
; CHECK: q[[Q:[0-9]+]] = vand([[SRA]],r{{[0-9]+}})
; CHECK: v{{[0-9]+}} = vmux(q[[Q]],v{{[0-9]+}},v{{[0-9]+}})

define internal fastcc <32 x i1> @cmp32(<32 x float> %a, float %b) #0 {
  %s0 = insertelement <32 x float> poison, float %b, i64 0
  %sp = shufflevector <32 x float> %s0,
                      <32 x float> poison,
                      <32 x i32> zeroinitializer
  %c = fcmp ogt <32 x float> %a, %sp
  ret <32 x i1> %c
}

define void @relu32(ptr %out, ptr %in) #0 {
  %v = load <32 x float>, ptr %in, align 128
  %m = call fastcc <32 x i1> @cmp32(<32 x float> %v,
                                    float 0.0)
  %r = select <32 x i1> %m,
              <32 x float> %v,
              <32 x float> zeroinitializer
  store <32 x float> %r, ptr %out, align 128
  ret void
}

attributes #0 = {
  noinline nounwind
  "target-cpu"="hexagonv75"
  "target-features"="+hvxv75,+hvx-length128b"
}
