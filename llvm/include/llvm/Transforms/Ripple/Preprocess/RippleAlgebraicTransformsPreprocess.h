//===----- RippleAlgebraicTransformsPreprocess.h - Undo algebraic combines
//---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Earlier pipeline stages (e.g. InstCombine's sin/cos fusion) may rewrite a
// pattern into a form Ripple cannot vectorize, such as fusing separate
// @llvm.sin/@llvm.cos calls into a single @llvm.sincos call returning a
// struct. Ripple cannot create vector types from aggregate-returning
// instructions, so this pass runs immediately before Ripple's vectorizer and
// undoes such algebraic combines, restoring the pre-combine form.
//
// This pass is meant to accumulate additional "undo" patterns over time as
// they are discovered; for now it only handles the sincos case above.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_VECTORIZE_RIPPLE_ALGEBRAIC_TRANSFORMS_PREPROCESS_H
#define LLVM_TRANSFORMS_VECTORIZE_RIPPLE_ALGEBRAIC_TRANSFORMS_PREPROCESS_H

#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Ripple/Ripple.h"

namespace llvm {
class TargetMachine;

class RippleAlgebraicTransformsPreprocessPass
    : public RequiredPassInfoMixin<RippleAlgebraicTransformsPreprocessPass> {
  TargetMachine *TM;
  Ripple::ProcessingStatus &PS;
  DenseSet<AssertingVH<Function>> &SpecializationsPending,
      &SpecializationsAvailable;

public:
  RippleAlgebraicTransformsPreprocessPass(
      TargetMachine *TM, Ripple::ProcessingStatus &PS,
      DenseSet<AssertingVH<Function>> &SpecializationsPending,
      DenseSet<AssertingVH<Function>> &SpecializationsAvailable)
      : TM(TM), PS(PS), SpecializationsPending(SpecializationsPending),
        SpecializationsAvailable(SpecializationsAvailable) {}
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};

} // namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_RIPPLE_ALGEBRAIC_TRANSFORMS_PREPROCESS_H
