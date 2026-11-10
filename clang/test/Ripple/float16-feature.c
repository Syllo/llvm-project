// REQUIRES: hexagon-registered-target || aarch64-registered-target || x86-registered-target
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv67 -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=0 -fsyntax-only -verify=no-fp16 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv67 -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=0 -fsyntax-only -verify=no-fp16 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv68 -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv68 -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv67 -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=0 -fsyntax-only -verify=no-fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv67 -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=0 -fsyntax-only -verify=no-fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv68 -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if hexagon-registered-target %{ %clang_cc1 -triple hexagon -target-cpu hexagonv68 -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if aarch64-registered-target %{ %clang_cc1 -triple aarch64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -std=c11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}
// RUN: %if x86-registered-target %{ %clang_cc1 -triple x86_64-unknown-linux-gnu -fenable-ripple -I %S/../../lib/Headers/ripple_include -x c++ -std=c++11 -DEXPECT_FP16=1 -fsyntax-only -verify=fp16 -DRIPPLE_VEC_HEADER=1 %s %}

#ifdef RIPPLE_VEC_HEADER
#include <__ripple_vec.h>
#else
#include <ripple.h>
#endif

// The Ripple feature macro is always defined; its value reports type support.
#ifndef __has_Float16__
#error "Ripple headers must define __has_Float16__"
#endif
#if __has_Float16__ != EXPECT_FP16
#error "Unexpected Ripple FP16 support for this target"
#endif

#if EXPECT_FP16
#if !defined(__FLT16_MANT_DIG__) || __FLT16_MANT_DIG__ != 11
#error "The compiler must expose binary16 when FP16 is supported"
#endif
#else
#ifdef __FLT16_MANT_DIG__
#error "The compiler must not expose binary16 when FP16 is unsupported"
#endif
#endif

// Check that the feature macros agree with the compiler's type support.
_Float16 fp16_value; // no-fp16-error {{_Float16 is not supported on this target}}
// fp16-no-diagnostics
