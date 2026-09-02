// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ not %clangxx --target=hexagon -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s %}
// RUN: %if x86-registered-target %{ not %clangxx --target=x86_64-unknown-linux-gnu -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s %}
// RUN: %if aarch64-registered-target %{ not %clangxx --target=aarch64-unknown-linux-gnu -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s %}

#include "ripple_test.h"

void reject_capturing_shuffle_mapper(float value) {
  size_t offset = 1;
  (void)ripple_shuffle(value, [offset](size_t index, size_t size) {
    return (index + offset) % size;
  });
}

// CHECK: error: cannot cast from type '{{.*}}' to pointer type '{{.*}}'
