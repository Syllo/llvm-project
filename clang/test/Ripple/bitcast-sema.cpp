// RUN: %clang_cc1 -fenable-ripple -fsyntax-only -verify %s

using Block = struct ripple_block_shape *;
using Vec = long long __attribute__((vector_size(32)));

void invalid(Block bs, Vec v, int n, void *p) {
  __builtin_ripple_from_fixed_vector(bs); // expected-error {{too few arguments}}
  __builtin_ripple_to_fixed_vector(1); // expected-error {{too few arguments}}
  __builtin_ripple_from_fixed_vector(p, v); // expected-error {{takes a ripple block shape}}
  __builtin_ripple_from_fixed_vector(bs, 1); // expected-error {{requires an integer or floating-point vector}}
  __builtin_ripple_to_fixed_vector(4, v); // expected-error {{requires an integer or floating-point scalar}}
  __builtin_ripple_to_fixed_vector(4, p); // expected-error {{requires an integer or floating-point scalar}}
  __builtin_ripple_to_fixed_vector(4, true); // expected-error {{requires an integer or floating-point scalar}}
  __builtin_ripple_to_fixed_vector(0, 1); // expected-error {{vector element count must be a positive integer constant}}
  __builtin_ripple_to_fixed_vector(-1, 1); // expected-error {{vector element count must be a positive integer constant}}
  __builtin_ripple_to_fixed_vector(n, 1); // expected-error {{vector element count must be a positive integer constant}}
  __builtin_ripple_to_fixed_vector(1073741824ULL, 1); // expected-error {{vector element count must be a positive integer constant no greater than 1073741823}}
  __builtin_ripple_to_fixed_vector(4294967296ULL, 1); // expected-error {{vector element count must be a positive integer constant}}
}

// This base supports vector types at the Ripple element-count limit.
using MaxVector = decltype(
    __builtin_ripple_to_fixed_vector(1073741823ULL, 1));
static_assert(__is_same(MaxVector, int __attribute__((vector_size(4294967292ULL)))),
              "maximum Ripple element count is accepted");

static_assert(__is_same(decltype(__builtin_ripple_from_fixed_vector(Block{}, Vec{})), long long), "element type");
static_assert(__is_same(decltype(__builtin_ripple_to_fixed_vector(1, 1)), int), "one element is scalar");
static_assert(__is_same(decltype(__builtin_ripple_to_fixed_vector(8, (short)1)), short __attribute__((vector_size(16)))), "no integer promotion");
static_assert(__is_same(decltype(__builtin_ripple_to_fixed_vector(4, 1.0f)), float __attribute__((vector_size(16)))), "no floating promotion");

template <class T, unsigned N> auto to_vector(T x) {
  return __builtin_ripple_to_fixed_vector(N, x);
}
static_assert(__is_same(decltype(to_vector<long long, 4>(0)), Vec), "dependent return type");
