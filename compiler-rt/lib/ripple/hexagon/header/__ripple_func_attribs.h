//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once
#include "vector_types.h"
#ifndef RIPPLE_VECTOR_CANDIDATE
#define RIPPLE_VECTOR_CANDIDATE __attribute__((ripple_vector_candidate))
#endif
#ifndef RIPPLE_MAYBE_UNUSED
#define RIPPLE_MAYBE_UNUSED __attribute__((unused))
#endif
#ifndef RIPPLE_STATIC_ASSERT
#ifdef __cplusplus
#define RIPPLE_STATIC_ASSERT static_assert
#else
#define RIPPLE_STATIC_ASSERT _Static_assert
#endif
#endif

// static inline keeps header definitions module-local in both C and C++,
// without requiring anonymous namespaces.
#ifndef RIPPLE_INTRIN_INLINE
#define RIPPLE_INTRIN_INLINE                                                   \
  static inline __attribute__((always_inline)) RIPPLE_MAYBE_UNUSED
#endif
#ifndef RIPPLE_CANDIDATE_INLINE
// Keep candidates module-local. The Ripple pass resolves internal C++-mangled
// candidate names to their source-level scalar names.
#define RIPPLE_CANDIDATE_INLINE RIPPLE_INTRIN_INLINE RIPPLE_VECTOR_CANDIDATE
#endif
