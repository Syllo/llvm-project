//====--------------------------- ripple_thd_it_serv.h --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Internal iteration server definition (part of the Ripple thd runtime)
//
//===----------------------------------------------------------------------===//

#pragma once

#include "ripple/ripple_thd_conventions.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief An iteration server shared state.
typedef struct _ripple_it_serv {
  // Current value of the iterator
  int32_t cur;
  // Exclusive bound (first value out of range).
  // If stride is >=0, this is the loop upper bound, otherwise its lower bound.
  int32_t end;
  /// Loop step
  int32_t stride;
  /// # threads that have yet to exit the previous instance of this it_serv
  int32_t to_exit;
  /// # threads that have yet to init the current instance of this it_serv
  int32_t to_init;
  // global epoch for all iteration servers (really just a 0/1 value)
  size_t epoch;
} it_serv_t;

#ifdef __cplusplus
}
#endif
