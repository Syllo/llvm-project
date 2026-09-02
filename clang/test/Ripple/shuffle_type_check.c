// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ not %clang --target=hexagon -DTEST_VALUE_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=VALUE %}
// RUN: %if hexagon-registered-target %{ not %clang --target=hexagon -DTEST_POINTER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=POINTER %}
// RUN: %if hexagon-registered-target %{ not %clang --target=hexagon -DTEST_MAPPER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=MAPPER %}
// RUN: %if x86-registered-target %{ not %clang --target=x86_64-unknown-linux-gnu -DTEST_VALUE_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=VALUE %}
// RUN: %if x86-registered-target %{ not %clang --target=x86_64-unknown-linux-gnu -DTEST_POINTER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=POINTER %}
// RUN: %if x86-registered-target %{ not %clang --target=x86_64-unknown-linux-gnu -DTEST_MAPPER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=MAPPER %}
// RUN: %if aarch64-registered-target %{ not %clang --target=aarch64-unknown-linux-gnu -DTEST_VALUE_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=VALUE %}
// RUN: %if aarch64-registered-target %{ not %clang --target=aarch64-unknown-linux-gnu -DTEST_POINTER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=POINTER %}
// RUN: %if aarch64-registered-target %{ not %clang --target=aarch64-unknown-linux-gnu -DTEST_MAPPER_MISMATCH -fsyntax-only -fenable-ripple %s 2>&1 | FileCheck %s --check-prefix=MAPPER %}

#include "ripple_test.h"

size_t shuffle_mapper(size_t index, size_t size) { return index % size; }

#ifdef TEST_VALUE_MISMATCH
void reject_mismatched_shuffle_values(int first, float second) {
  (void)ripple_shuffle_pair(first, second, shuffle_mapper);
}
#endif

#ifdef TEST_POINTER_MISMATCH
void reject_mismatched_shuffle_pointers(int *first, float *second) {
  (void)ripple_shuffle_pair_p(first, second, shuffle_mapper);
}
#endif

#ifdef TEST_MAPPER_MISMATCH
int wrong_shuffle_mapper(int index, int size) { return index % size; }

void reject_mismatched_shuffle_mapper(float value) {
  (void)ripple_shuffle(value, wrong_shuffle_mapper);
}
#endif

// VALUE: ripple_shuffle_pair operands must have the same type
// POINTER: ripple_shuffle_pair operands must have the same type
// MAPPER: ripple_shuffle mapper must have type size_t (*)(size_t, size_t)
