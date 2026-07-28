//===----- RippleAlgebraicTransformsPreprocess.cpp - Undo algebraic
// combines ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Ripple/Preprocess/RippleAlgebraicTransformsPreprocess.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "ripple-algebraic-transforms-preprocess"

////////////////////////////////////////////////////////////////////////////////
///               RippleAlgebraicTransformsPreprocessPass                    ///
////////////////////////////////////////////////////////////////////////////////

namespace {

/// @brief Computes the per-instruction Ripple shapes for \p F by running
/// shape propagation, mirroring the helper used by RippleSESEPass. This lets
/// this pre-processing pass query the ripple shape of an instruction before
/// RipplePass itself runs.
/// @return Error, if any, encountered during shape propagation. On success
/// but when specialization is pending, returns an empty map (this pass will
/// simply run again once the specialization is available).
Expected<std::map<AssertingVH<const Instruction>, TensorShape>>
getInstructionToRippleShape(
    TargetMachine *TM, Function &F, FunctionAnalysisManager &AM,
    Ripple::ProcessingStatus &PS,
    DenseSet<AssertingVH<Function>> &SpecializationsPending,
    DenseSet<AssertingVH<Function>> &SpecializationsAvailable) {
  // WIP machine model w/ 1 vector dimension
  std::vector<std::pair<Ripple::PEIdentifier, Ripple::DimType>> DimTypes = {
      {0, Ripple::VectorDimension}};

  Ripple R(TM, F, AM, DimTypes, PS, SpecializationsPending,
           SpecializationsAvailable);
  if (R.tensorRank() == 0) {
    // No ripple usage.
    return std::map<AssertingVH<const Instruction>, TensorShape>();
  }

  R.initFuncRPOT();

  R.registerInModuleRippleCandidates();

  bool WaitingForSpecialization = false;
  if (Error E = R.propagateShapes(WaitingForSpecialization))
    return std::move(E);

  if (WaitingForSpecialization)
    return std::map<AssertingVH<const Instruction>, TensorShape>();

  return R.getInstructionToRippleShape();
}

} // namespace

// Entrypoint for this pass.
PreservedAnalyses
RippleAlgebraicTransformsPreprocessPass::run(Function &F,
                                             FunctionAnalysisManager &AM) {
  // Shape propagation can request specializations. Avoid doing that work for
  // functions with no combines to undo.
  if (none_of(instructions(F), [](const Instruction &I) {
        const auto *II = dyn_cast<IntrinsicInst>(&I);
        return II && II->getIntrinsicID() == Intrinsic::sincos;
      }))
    return PreservedAnalyses::all();

  PreservedAnalyses PA = PreservedAnalyses::none();
  PA.preserve<TargetLibraryAnalysis>();

  LLVM_DEBUG(dbgs() << "Applying RippleAlgebraicTransformsPreprocessPass to '"
                    << F.getName() << "'\n");

  Expected<std::map<AssertingVH<const Instruction>, TensorShape>>
      InstructionShapesOrErr = getInstructionToRippleShape(
          TM, F, AM, PS, SpecializationsPending, SpecializationsAvailable);
  if (Error E = InstructionShapesOrErr.takeError()) {
    // Shape propagation failed; let RipplePass surface the diagnostic as
    // usual and skip this preprocessing step for this function.
    llvm::consumeError(std::move(E));
    return PA;
  }

  std::map<AssertingVH<const Instruction>, TensorShape> InstructionShapes =
      std::move(*InstructionShapesOrErr);

  auto isNonScalar = [&](const Instruction *I) {
    auto It = InstructionShapes.find(I);
    return It != InstructionShapes.end() && !It->second.isScalar();
  };

  // Undo InstCombine's sin/cos -> sincos fusion: Ripple cannot vectorize the
  // aggregate-returning @llvm.sincos intrinsic, so replace
  // `extractvalue(sincos(x), 0)` with `sin(x)` and
  // `extractvalue(sincos(x), 1)` with `cos(x)`, but only when the instruction
  // actually has a non-scalar ripple shape (otherwise there is nothing for
  // Ripple to vectorize and the sincos call can be left alone).
  //
  // We only decide what to do here: InstructionShapes is keyed by
  // AssertingVH, so it must be cleared before we start erasing instructions
  // below, or those AssertingVHs will fire as soon as their tracked
  // instruction is deleted.
  struct SinCosSplit {
    ExtractValueInst *EVI;
    Instruction *SinCos;
    Value *Arg;
    Intrinsic::ID ID;
  };
  SmallVector<SinCosSplit, 8> ToSplit;
  SmallPtrSet<Instruction *, 8> SinCosCalls;

  for (auto &I : instructions(F)) {
    if (!isNonScalar(&I))
      continue;

    Value *Arg = nullptr;
    Instruction *SinCos = nullptr;

    if (match(&I, m_ExtractValue<0>(m_Instruction(
                      SinCos, m_Intrinsic<Intrinsic::sincos>(m_Value(Arg)))))) {
      ToSplit.push_back(
          {cast<ExtractValueInst>(&I), SinCos, Arg, Intrinsic::sin});
      SinCosCalls.insert(SinCos);
    } else if (match(&I, m_ExtractValue<1>(m_Instruction(
                             SinCos,
                             m_Intrinsic<Intrinsic::sincos>(m_Value(Arg)))))) {
      ToSplit.push_back(
          {cast<ExtractValueInst>(&I), SinCos, Arg, Intrinsic::cos});
      SinCosCalls.insert(SinCos);
    }
  }

  InstructionShapes.clear();

  IRBuilder<> Builder(F.getContext());
  for (SinCosSplit &Split : ToSplit) {
    Builder.SetInsertPoint(Split.EVI);
    // Preserve the accuracy bound carried by the fused sincos call.
    Builder.setDefaultFPMathTag(
        Split.SinCos->getMetadata(LLVMContext::MD_fpmath));
    Value *Replacement =
        Builder.CreateUnaryIntrinsic(Split.ID, Split.Arg, Split.SinCos);
    auto IIt = Split.EVI->getIterator();
    ReplaceInstWithValue(IIt, Replacement);
  }

  // Any @llvm.sincos calls left with no remaining users (both extractvalues
  // rewritten above) are now dead.
  for (Instruction *SinCos : SinCosCalls) {
    if (SinCos->use_empty())
      SinCos->eraseFromParent();
  }

  return PA;
}
