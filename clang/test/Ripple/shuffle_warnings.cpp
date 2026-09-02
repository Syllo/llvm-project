// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clangxx --target=hexagon -std=c++11 -fsyntax-only -fenable-ripple -Weverything -Werror %s %}
// RUN: %if x86-registered-target %{ %clangxx --target=x86_64-unknown-linux-gnu -std=c++11 -fsyntax-only -fenable-ripple -Weverything -Werror %s %}
// RUN: %if aarch64-registered-target %{ %clangxx --target=aarch64-unknown-linux-gnu -std=c++11 -fsyntax-only -fenable-ripple -Weverything -Werror %s %}

#include <ripple.h>

typedef __SIZE_TYPE__ size_t;

static size_t shuffle_mapper(size_t index, size_t size) {
  return index % size;
}

int main() {
  float value = 0;
  (void)ripple_shuffle(value, shuffle_mapper);
}
