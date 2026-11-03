; REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
; RUN: %if hexagon-registered-target %{ opt -S -mtriple=hexagon -mcpu=hexagonv81 -mattr=+hvx-length128b,+hvxv81,+v81 \
; RUN:   -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - \
; RUN:   | FileCheck %s --check-prefixes=CHECK,HVX %}
; RUN: %if aarch64-registered-target %{ opt -S -mtriple=aarch64 -mattr=+neon \
; RUN:   -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - \
; RUN:   | FileCheck %s --check-prefixes=CHECK,NEON %}
; RUN: %if x86-registered-target %{ opt -S -mtriple=x86_64 -mattr=+avx2 \
; RUN:   -passes='module(ripple),verify' -ripple-pad-to-target-simd -ripple-run-only-ripple-passes %s -o - \
; RUN:   | FileCheck %s --check-prefixes=CHECK,AVX2 %}

; Reinterpret packs pairs of i16 lanes into i32 lanes. Even when both vectors
; are already SIMD-aligned, their lane counts differ.
define void @aligned_pack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 128, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 64, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %r = call i32 @llvm.ripple.reinterp.u32.i16(ptr %dst, i16 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i32, ptr %output, i32 %j
  store i32 %r, ptr %q, align 4
  ret void
}
; CHECK-LABEL: define void @aligned_pack(
; CHECK: bitcast <128 x i16> {{.*}} to <64 x i32>
; CHECK: store <64 x i32>

; The inverse aligned conversion must likewise retain its distinct lane counts.
define void @aligned_unpack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 64, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 128, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i32, ptr %input, i32 %i
  %v = load i32, ptr %p, align 4
  %r = call i16 @llvm.ripple.reinterp.u16.i32(ptr %dst, i32 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i16, ptr %output, i32 %j
  store i16 %r, ptr %q, align 2
  ret void
}
; CHECK-LABEL: define void @aligned_unpack(
; CHECK: bitcast <64 x i32> {{.*}} to <128 x i16>
; CHECK: store <128 x i16>

; Padding must preserve total bit width rather than use a common lane count.
; 38 i16 lanes / 19 i32 lanes require padding on all three targets.
define void @padded_pack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 38, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %r = call i32 @llvm.ripple.reinterp.u32.i16(ptr %dst, i16 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i32, ptr %output, i32 %j
  store i32 %r, ptr %q, align 4
  ret void
}
; CHECK-LABEL: define void @padded_pack(
; HVX: bitcast <64 x i16> {{.*}} to <32 x i32>
; HVX: call void @llvm.masked.store.v32i32.p0(
; NEON: bitcast <40 x i16> {{.*}} to <20 x i32>
; NEON: call void @llvm.masked.store.v20i32.p0(
; AVX2: bitcast <48 x i16> {{.*}} to <24 x i32>
; AVX2: call void @llvm.masked.store.v24i32.p0(

define void @padded_unpack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 38, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i32, ptr %input, i32 %i
  %v = load i32, ptr %p, align 4
  %r = call i16 @llvm.ripple.reinterp.u16.i32(ptr %dst, i32 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i16, ptr %output, i32 %j
  store i16 %r, ptr %q, align 2
  ret void
}
; CHECK-LABEL: define void @padded_unpack(
; HVX: bitcast <32 x i32> {{.*}} to <64 x i16>
; HVX: call void @llvm.masked.store.v64i16.p0(
; NEON: bitcast <20 x i32> {{.*}} to <40 x i16>
; NEON: call void @llvm.masked.store.v40i16.p0(
; AVX2: bitcast <24 x i32> {{.*}} to <48 x i16>
; AVX2: call void @llvm.masked.store.v48i16.p0(

; Equal element widths retain equal lane counts when padded.
define void @same_width_bitcast(ptr %input, ptr %output) #0 {
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr float, ptr %input, i32 %i
  %v = load float, ptr %p, align 4
  %r = bitcast float %v to i32
  %q = getelementptr i32, ptr %output, i32 %i
  store i32 %r, ptr %q, align 4
  ret void
}
; CHECK-LABEL: define void @same_width_bitcast(
; HVX: bitcast <32 x float> {{.*}} to <32 x i32>
; HVX: call void @llvm.masked.store.v32i32.p0(
; NEON: bitcast <20 x float> {{.*}} to <20 x i32>
; NEON: call void @llvm.masked.store.v20i32.p0(
; AVX2: bitcast <24 x float> {{.*}} to <24 x i32>
; AVX2: call void @llvm.masked.store.v24i32.p0(

; Element-wise casts with different element widths must still preserve lane
; counts, rather than adopt the bitcast rule of preserving total bit width.
define void @elementwise_casts(ptr %input, ptr %output) #0 {
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %wide = zext i16 %v to i32
  %shift = lshr i32 %wide, 8
  %r = trunc i32 %shift to i8
  %q = getelementptr i8, ptr %output, i32 %i
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @elementwise_casts(
; HVX: zext <64 x i16> {{.*}} to <64 x i32>
; HVX: trunc <128 x i32> {{.*}} to <128 x i8>
; HVX: call void @llvm.masked.store.v128i8.p0(
; NEON: zext <24 x i16> {{.*}} to <24 x i32>
; NEON: trunc <32 x i32> {{.*}} to <32 x i8>
; NEON: call void @llvm.masked.store.v32i8.p0(
; AVX2: zext <32 x i16> {{.*}} to <32 x i32>
; AVX2: trunc <32 x i32> {{.*}} to <32 x i8>
; AVX2: call void @llvm.masked.store.v32i8.p0(

; Predicate vectors can have a smaller natural padded bit width than data
; vectors. Extend the predicate source to the destination's requested width.
define void @predicate_pack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 40, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 5, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i1, ptr %input, i32 %i
  %v = load i1, ptr %p, align 1
  %r = call i8 @llvm.ripple.reinterp.u8.i1(ptr %dst, i1 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i8, ptr %output, i32 %j
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @predicate_pack(
; HVX: bitcast <1024 x i1> {{.*}} to <128 x i8>
; HVX: call void @llvm.masked.store.v128i8.p0(
; NEON: bitcast <128 x i1> {{.*}} to <16 x i8>
; NEON: call void @llvm.masked.store.v16i8.p0(
; AVX2: bitcast <256 x i1> {{.*}} to <32 x i8>
; AVX2: call void @llvm.masked.store.v32i8.p0(

; The destination's natural bit width need not be divisible by the source's
; element width. Round the destination down to whole groups before deriving
; source lanes, then restore its natural padded length after the bitcast.
define void @odd_width_pack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 8, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i3, ptr %input, i32 %i
  %v = load i3, ptr %p, align 1
  %r = call i8 @llvm.ripple.reinterp.u8.i3(ptr %dst, i3 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i8, ptr %output, i32 %j
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @odd_width_pack(
; HVX: bitcast <336 x i3> {{.*}} to <126 x i8>
; HVX: shufflevector <126 x i8> {{.*}} <128 x i32> <i32 0,{{.*}}i32 125, i32 poison, i32 poison>
; HVX: call void @llvm.masked.store.v128i8.p0(
; NEON: bitcast <40 x i3> {{.*}} to <15 x i8>
; NEON: shufflevector <15 x i8> {{.*}} <16 x i32> <i32 0,{{.*}}i32 14, i32 poison>
; NEON: call void @llvm.masked.store.v16i8.p0(
; AVX2: bitcast <80 x i3> {{.*}} to <30 x i8>
; AVX2: shufflevector <30 x i8> {{.*}} <32 x i32> <i32 0,{{.*}}i32 29, i32 poison, i32 poison>
; AVX2: call void @llvm.masked.store.v32i8.p0(

; Both element widths are composite and share a nontrivial factor:
; gcd(12, 8) = 4, so destination lanes must be grouped in multiples of 12/4 = 3.
; Each group of three i8 lanes matches two i12 lanes (24 bits).
define void @composite_width_pack(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 3, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i12, ptr %input, i32 %i
  %v = load i12, ptr %p, align 1
  %r = call i8 @llvm.ripple.reinterp.u8.i12(ptr %dst, i12 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i8, ptr %output, i32 %j
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @composite_width_pack(
; HVX: bitcast <84 x i12> {{.*}} to <126 x i8>
; HVX: shufflevector <126 x i8> {{.*}} <128 x i32> <i32 0,{{.*}}i32 125, i32 poison, i32 poison>
; HVX: call void @llvm.masked.store.v128i8.p0(
; NEON: bitcast <10 x i12> {{.*}} to <15 x i8>
; NEON: shufflevector <15 x i8> {{.*}} <16 x i32> <i32 0,{{.*}}i32 14, i32 poison>
; NEON: call void @llvm.masked.store.v16i8.p0(
; AVX2: bitcast <20 x i12> {{.*}} to <30 x i8>
; AVX2: shufflevector <30 x i8> {{.*}} <32 x i32> <i32 0,{{.*}}i32 29, i32 poison, i32 poison>
; AVX2: call void @llvm.masked.store.v32i8.p0(

; Rounding down can reach exactly the original destination length. All 126
; original bytes must survive, with only the final two padded lanes masked out.
define void @round_down_preserves_all_lanes(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 84, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 126, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i12, ptr %input, i32 %i
  %v = load i12, ptr %p, align 1
  %r = call i8 @llvm.ripple.reinterp.u8.i12(ptr %dst, i12 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i8, ptr %output, i32 %j
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @round_down_preserves_all_lanes(
; CHECK: [[CAST:%.*]] = bitcast <84 x i12> {{.*}} to <126 x i8>
; CHECK: [[PAD:%.*]] = shufflevector <126 x i8> [[CAST]], <126 x i8> poison, <128 x i32> <i32 0,{{.*}}i32 125, i32 poison, i32 poison>
; CHECK: call void @llvm.masked.store.v128i8.p0(<128 x i8> [[PAD]], ptr align 1 {{.*}}, <128 x i1> <i1 true,{{.*}}i1 true, i1 false, i1 false>)

; The source can also have a larger natural padded bit width. Its extra padded
; lanes can be trimmed while retaining every original bit before the bitcast.
; Previously HVX tried to bitcast 1364 i3 lanes (4092 bits) to 511 i8 lanes
; (4088 bits), truncating the division by the destination element width.
define void @source_padded_wider(ptr %input, ptr %output) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 1024, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 384, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i3, ptr %input, i32 %i
  %v = load i3, ptr %p, align 1
  %r = call i8 @llvm.ripple.reinterp.u8.i3(ptr %dst, i3 %v)
  %j = call i32 @llvm.ripple.block.index.i32(ptr %dst, i32 0)
  %q = getelementptr i8, ptr %output, i32 %j
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @source_padded_wider(
; HVX: shufflevector <1364 x i3> {{.*}} <1024 x i32>
; NEON: shufflevector <1050 x i3> {{.*}} <1024 x i32>
; AVX2: shufflevector <1105 x i3> {{.*}} <1024 x i32>
; CHECK: bitcast <1024 x i3> {{.*}} to <384 x i8>
; CHECK: call void @llvm.masked.store.v384i8.p0(

; Native scalar/vector bitcasts retain scalar Ripple shape and their exact
; types when their results feed a computation that needs SIMD padding.
define <2 x i16> @native_scalar_to_vector(i32 %bits, ptr %input, ptr %output) #0 {
  %native = bitcast i32 %bits to <2 x i16>
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %ripple = call i16 @llvm.ripple.from.fixed.vector.v2i16(ptr %bs, <2 x i16> %native)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %sum = add i16 %v, %ripple
  %q = getelementptr i16, ptr %output, i32 %i
  store i16 %sum, ptr %q, align 2
  ret <2 x i16> %native
}
; CHECK-LABEL: define <2 x i16> @native_scalar_to_vector(
; CHECK: [[NATIVE:%.*]] = bitcast i32 %bits to <2 x i16>
; CHECK: [[RIPPLE:%.*]] = bitcast <2 x i16> [[NATIVE]] to <2 x i16>
; CHECK: [[PAD:%.*]] = shufflevector <2 x i16> [[RIPPLE]], <2 x i16> poison, <{{[0-9]+}} x i32> <i32 0, i32 1, i32 poison,
; HVX: call <64 x i16> @llvm.masked.load.v64i16.p0(
; NEON: call <8 x i16> @llvm.masked.load.v8i16.p0(
; AVX2: call <16 x i16> @llvm.masked.load.v16i16.p0(
; CHECK: [[SUM:%.*]] = add <{{[0-9]+}} x i16> {{%.*}}, [[PAD]]
; CHECK: call void @llvm.masked.store.{{.*}}(<{{[0-9]+}} x i16> [[SUM]],
; CHECK: ret <2 x i16> [[NATIVE]]

define i32 @native_vector_to_scalar(<2 x i16> %native, ptr %input, ptr %output) #0 {
  %bits = bitcast <2 x i16> %native to i32
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr i32, ptr %input, i32 %i
  %v = load i32, ptr %p, align 4
  %sum = add i32 %v, %bits
  %q = getelementptr i32, ptr %output, i32 %i
  store i32 %sum, ptr %q, align 4
  ret i32 %bits
}
; CHECK-LABEL: define i32 @native_vector_to_scalar(
; CHECK: [[BITS:%.*]] = bitcast <2 x i16> %native to i32
; CHECK: [[INSERT:%.*]] = insertelement <{{[0-9]+}} x i32> poison, i32 [[BITS]], i64 0
; CHECK: [[SPLAT:%.*]] = shufflevector <{{[0-9]+}} x i32> [[INSERT]],
; HVX: call <32 x i32> @llvm.masked.load.v32i32.p0(
; NEON: call <20 x i32> @llvm.masked.load.v20i32.p0(
; AVX2: call <24 x i32> @llvm.masked.load.v24i32.p0(
; CHECK: [[SUM:%.*]] = add <{{[0-9]+}} x i32> {{%.*}}, [[SPLAT]]
; CHECK: call void @llvm.masked.store.{{.*}}(<{{[0-9]+}} x i32> [[SUM]],
; CHECK: ret i32 [[BITS]]

; Scalar reinterpret can produce a scalar/vector bitcast. It must bypass the
; vector/vector padding helper's assert and retain its exact 32-bit width.
define void @scalar_reinterpret(i32 %bits, ptr %output) #0 {
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 4, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %r = call i8 @llvm.ripple.reinterp.u8.i32(ptr %bs, i32 %bits)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %q = getelementptr i8, ptr %output, i32 %i
  store i8 %r, ptr %q, align 1
  ret void
}
; CHECK-LABEL: define void @scalar_reinterpret(
; CHECK: [[SCALAR:%.*]] = bitcast i32 %bits to <4 x i8>
; HVX: [[PAD:%.*]] = shufflevector <4 x i8> [[SCALAR]], <4 x i8> poison, <128 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison,
; NEON: [[PAD:%.*]] = shufflevector <4 x i8> [[SCALAR]], <4 x i8> poison, <16 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison,
; AVX2: [[PAD:%.*]] = shufflevector <4 x i8> [[SCALAR]], <4 x i8> poison, <32 x i32> <i32 0, i32 1, i32 2, i32 3, i32 poison,
; CHECK: call void @llvm.masked.store.{{.*}}(<{{[0-9]+}} x i8> [[PAD]], ptr align 1 {{.*}}, <{{[0-9]+}} x i1> <i1 true, i1 true, i1 true, i1 true, i1 false,
; CHECK: ret void

; Varying reinterpret must reach the padding helper with vectors on both
; sides. Check the bitcast types explicitly so a scalar/vector lowering fails.
define void @vector_reinterpret(ptr %input, ptr %output) #0 {
  %bs = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 19, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %bs, i32 0)
  %p = getelementptr i32, ptr %input, i32 %i
  %v = load i32, ptr %p, align 4
  %r = call float @llvm.ripple.reinterp.f32.i32(ptr %bs, i32 %v)
  %q = getelementptr float, ptr %output, i32 %i
  store float %r, ptr %q, align 4
  ret void
}
; CHECK-LABEL: define void @vector_reinterpret(
; HVX: [[VECTOR:%.*]] = bitcast <32 x i32> {{%.*}} to <32 x float>
; NEON: [[VECTOR:%.*]] = bitcast <20 x i32> {{%.*}} to <20 x float>
; AVX2: [[VECTOR:%.*]] = bitcast <24 x i32> {{%.*}} to <24 x float>
; CHECK: call void @llvm.masked.store.{{.*}}(<{{[0-9]+}} x float> [[VECTOR]],
; CHECK: ret void

; SIMD padding must extract the original two source lanes before a
; vector-to-scalar bitcast, preserving its exact 32-bit input width.
define i32 @padded_vector_to_scalar_bitcast(ptr %input) #0 {
  %src = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 2, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %dst = call ptr @llvm.ripple.block.setshape.i32(i32 0, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1, i32 1)
  %i = call i32 @llvm.ripple.block.index.i32(ptr %src, i32 0)
  %p = getelementptr i16, ptr %input, i32 %i
  %v = load i16, ptr %p, align 2
  %r = call i32 @llvm.ripple.reinterp.u32.i16(ptr %dst, i16 %v)
  ret i32 %r
}
; CHECK-LABEL: define i32 @padded_vector_to_scalar_bitcast(
; HVX: [[LOAD:%.*]] = call <64 x i16> @llvm.masked.load.v64i16.p0(
; NEON: [[LOAD:%.*]] = call <8 x i16> @llvm.masked.load.v8i16.p0(
; AVX2: [[LOAD:%.*]] = call <16 x i16> @llvm.masked.load.v16i16.p0(
; CHECK: [[TRIM:%.*]] = call <2 x i16> @llvm.vector.extract.v2i16.{{.*}}(<{{[0-9]+}} x i16> [[LOAD]], i64 0)
; CHECK: [[SCALAR:%.*]] = bitcast <2 x i16> [[TRIM]] to i32
; CHECK: ret i32 [[SCALAR]]

declare ptr @llvm.ripple.block.setshape.i32(i32 immarg, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32)
declare i32 @llvm.ripple.block.index.i32(ptr, i32 immarg)
declare i16 @llvm.ripple.from.fixed.vector.v2i16(ptr, <2 x i16>)
declare float @llvm.ripple.reinterp.f32.i32(ptr, i32)
declare i8 @llvm.ripple.reinterp.u8.i32(ptr, i32)
declare i32 @llvm.ripple.reinterp.u32.i16(ptr, i16)
declare i16 @llvm.ripple.reinterp.u16.i32(ptr, i32)
declare i8 @llvm.ripple.reinterp.u8.i1(ptr, i1)
declare i8 @llvm.ripple.reinterp.u8.i3(ptr, i3)
declare i8 @llvm.ripple.reinterp.u8.i12(ptr, i12)
attributes #0 = { nounwind }
