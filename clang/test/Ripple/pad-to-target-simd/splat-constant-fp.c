// REQUIRES: hexagon-registered-target || x86-registered-target || aarch64-registered-target

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=NOPAD %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=NOPAD %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -emit-llvm %s -o - 2>&1 | FileCheck %s --check-prefix=NOPAD %}

// RUN: %if hexagon-registered-target %{ %clang -ffreestanding -S --target=hexagon -mhvx -mv81 -mhvx-length=128B -O2 -fenable-ripple -fdisable-ripple-lib -mllvm -ripple-pad-to-target-simd -mllvm -ripple-run-only-ripple-passes -mllvm -print-after=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=PAD,PAD-HEX %}
// RUN: %if x86-registered-target %{ %clang -ffreestanding -S --target=x86_64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -mllvm -ripple-pad-to-target-simd -mllvm -ripple-run-only-ripple-passes -mllvm -print-after=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=PAD,PAD-V4 %}
// RUN: %if aarch64-registered-target %{ %clang -ffreestanding -S --target=aarch64-unknown-linux-gnu -O2 -fenable-ripple -fdisable-ripple-lib -mllvm -ripple-pad-to-target-simd -mllvm -ripple-run-only-ripple-passes -mllvm -print-after=ripple -emit-llvm %s -o /dev/null 2>&1 | FileCheck %s --check-prefixes=PAD,PAD-V4 %}

#include "../ripple_test.h"

void splat_constant_fp(float *a, float *out) {
  ripple_block_t BS = ripple_set_block_shape(0, 3);
  size_t v0 = ripple_id(BS, 0);
  out[v0] = a[v0] + 4.2f;
}

// NOPAD-LABEL: define {{.*}}void @splat_constant_fp(
// NOPAD: load <3 x float>
// NOPAD: fadd <3 x float> %{{.*}}, splat (float 4.200000e+00)
// NOPAD: store <3 x float>
// NOPAD: ret void

// PAD-LABEL: define {{.*}}void @splat_constant_fp(
// PAD-HEX: @llvm.masked.load.v32f32
// PAD-HEX: fadd <32 x float> %{{.*}}, splat (float 4.200000e+00)
// PAD-HEX: @llvm.masked.store.v32f32
// PAD-V4: @llvm.masked.load.v4f32
// PAD-V4: fadd <4 x float> %{{.*}}, splat (float 4.200000e+00)
// PAD-V4: @llvm.masked.store.v4f32
// PAD: ret void
