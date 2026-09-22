//====--------------------- ripple_thd_conventions.h ----------------------===//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Declarations common to multiple targets for ripple/thread.h
//
//===----------------------------------------------------------------------===//

#pragma once

// A default constant for the default threading PE
#define RIPPLE_THD 0
#define RIPPLE_THD_MAX_U 4294967295u

/// Maximum number of thread block dimensions supported by Ripple
#define RIPPLE_MAX_THD_BLOCK_DIMS 3
/// Max # of possible combinations of thread block dimensions
#define RIPPLE_MAX_THD_BLOCK_DIMS_COMBO 8

// _____________________________________________________________________________

#ifndef RIPPLE_THD_SCALAR
#ifdef __hexagon__
#define RIPPLE_THD_SCALAR __attribute__((target("no-hvx")))
#else
#define RIPPLE_THD_SCALAR
#endif
#endif

// _____________________ Portable C / C++ abstractions _________________________

#ifdef __cplusplus
#define RTHD_RCAST(T, V) reinterpret_cast<T>(V)
#define RTHD_NULL nullptr
#else
#define RTHD_RCAST(T, V) (T)(V)
#define RTHD_NULL NULL
#endif

// ______________________________ Barriers _____________________________________

// Max number of barrier objects to support arbitrary-dimension
// barriers for 1d to 4d
#define RIPPLE_THD_MAX_BARRIER_1D(n_thd) (1)
#define RIPPLE_THD_MAX_BARRIER_2D(n_thd) (((n_thd + 1) / 2) + 3)
#define RIPPLE_THD_MAX_BARRIER_3D(n_thd) (((5 * (n_thd) + 3) / 4) + 9)
#define RIPPLE_THD_MAX_BARRIER_4D(n_thd) (((9 * (n_thd) + 3) / 4) + 21)
