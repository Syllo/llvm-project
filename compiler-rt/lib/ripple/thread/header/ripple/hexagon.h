//===--------------------------- hexagon.h --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Hexagon-specific API.
//
//===----------------------------------------------------------------------===//

#pragma once

extern int qurt_hvx_get_units(void);
extern unsigned qurt_thread_get_id(void);

/// @brief number of HVX threads on this platform (from QuRT)
#define ripple_num_hvx() ((qurt_hvx_get_units() >> 8) & 0xFFu)
