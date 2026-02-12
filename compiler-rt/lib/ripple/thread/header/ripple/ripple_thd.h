//---====------------------------ ripple_thd.h ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Overarching header for ripple_thd. Compile with the definitions below to
// select which underlying runtime to use.
//
//===----------------------------------------------------------------------===//

#if defined(RIPPLE_THD_USE_QHPI)
#include <ripple/ripple_thd_qhpi.h>

#elif defined(RIPPLE_THD_USE_QURT)
#include <ripple/ripple_thd_qurt.h>

#else
#error Please define a macro to specify your Ripple thread backend: \
RIPPLE_THD_USE_QURT, RIPPLE_THD_USE_QHPI
#endif
