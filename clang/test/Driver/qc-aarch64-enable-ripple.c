// RUN: %clang -target aarch64-linux-gnu -fenable-ripple  %s -### -O2 -c 2>&1 | FileCheck %s
//
// Enable SVE/SME intrinsic selection for Ripple
//
// CHECK: -aarch64-enable-sve-sme-intrinsic-selection
