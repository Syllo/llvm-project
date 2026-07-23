//===- RippleAPIPreprocess.h - Early Ripple API preprocessing ---*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_RIPPLE_PREPROCESS_RIPPLE_API_PREPROCESS_H
#define LLVM_TRANSFORMS_RIPPLE_PREPROCESS_RIPPLE_API_PREPROCESS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class RippleAPIPreprocessPass : public PassInfoMixin<RippleAPIPreprocessPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

  static bool isRequired() { return true; }
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_RIPPLE_PREPROCESS_RIPPLE_API_PREPROCESS_H
