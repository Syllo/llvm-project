//===-- RippleAPIPreprocess.cpp - Early Ripple API preprocessing ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Ripple/Preprocess/RippleAPIPreprocess.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/ConstantRange.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsRipple.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Transforms/Ripple/Ripple.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <cstdint>
#include <optional>

using namespace llvm;

#define DEBUG_TYPE "ripple-api-preprocess"

// This pass exposes static information from Ripple API calls before the main
// Ripple transform consumes them. In optimized pipelines it runs after early
// frontend cleanup, but before scalar optimization and RippleModulePass, so
// facts such as constant block sizes, block-index ranges, and no-op API forms
// are visible to IPSCCP/SCCP, InstCombine, SimplifyCFG, JumpThreading/CVP, and
// SCEV users.
//
// At O0 the API shape values can still be materialized through memory. The
// MemorySSA lookup keeps those cases useful without requiring mem2reg/SROA.
namespace {

struct RangeBounds {
  APInt Lower;
  APInt Upper;
};

std::optional<uint64_t> getConstantOperandValue(const Instruction *I,
                                                unsigned Index) {
  if (Index >= I->getNumOperands())
    return std::nullopt;
  auto *C = dyn_cast<ConstantInt>(I->getOperand(Index));
  if (!C || C->getValue().getActiveBits() > 64)
    return std::nullopt;
  return C->getZExtValue();
}

std::optional<uint64_t> getBlockShapeDimSize(IntrinsicInst *SetShape,
                                             uint64_t Dimension) {
  // Operand 0 is the PE identifier, so the dimension sizes start at 1.
  if (Dimension >= SetShape->arg_size() - 1)
    return std::nullopt;
  return getConstantOperandValue(SetShape, Dimension + 1);
}

// Attach the unsigned interval [0, Upper) as both !range metadata and a
// range() return attribute, so consumers that key off either form see it.
bool setUnsignedRangeAnnotation(IntrinsicInst *Index, uint64_t Upper) {
  if (Index->getMetadata(LLVMContext::MD_range) ||
      Index->getRetAttr(Attribute::Range).isValid())
    return false;

  auto *IntTy = dyn_cast<IntegerType>(Index->getType());
  if (!IntTy || Upper == 0)
    return false;

  unsigned BitWidth = IntTy->getBitWidth();
  if (BitWidth < 64 && Upper >= (uint64_t(1) << BitWidth))
    return false;

  APInt LowerValue(BitWidth, 0);
  APInt UpperValue(BitWidth, Upper);

  MDBuilder MDB(Index->getContext());
  MDNode *Range = MDB.createRange(LowerValue, UpperValue);
  if (!Range)
    return false;

  Index->setMetadata(LLVMContext::MD_range, Range);
  Index->addRangeRetAttr(ConstantRange(LowerValue, UpperValue));
  return true;
}

std::optional<RangeBounds> getSimpleRangeBounds(Value *V, Type *ExpectedTy) {
  auto *IntTy = dyn_cast<IntegerType>(ExpectedTy);
  if (!IntTy)
    return std::nullopt;

  if (auto *C = dyn_cast<ConstantInt>(V)) {
    if (C->getType() != IntTy)
      return std::nullopt;
    APInt LowerValue = C->getValue();
    APInt UpperValue = LowerValue + 1;
    if (UpperValue.ule(LowerValue))
      return std::nullopt;
    return RangeBounds{LowerValue, UpperValue};
  }

  auto *I = dyn_cast<Instruction>(V);
  if (!I || I->getType() != ExpectedTy)
    return std::nullopt;

  std::optional<RangeBounds> Result;

  // Read the range() return attribute if this is a call.
  if (auto *CB = dyn_cast<CallBase>(I)) {
    Attribute RangeAttr = CB->getRetAttr(Attribute::Range);
    if (RangeAttr.isValid()) {
      ConstantRange CR = RangeAttr.getValueAsConstantRange();
      if (CR.getBitWidth() == IntTy->getBitWidth() && !CR.isFullSet() &&
          !CR.isEmptySet() && !CR.isWrappedSet())
        Result = RangeBounds{CR.getLower(), CR.getUpper()};
    }
  }

  // Read !range metadata.
  MDNode *Range = I->getMetadata(LLVMContext::MD_range);
  if (Range && Range->getNumOperands() == 2) {
    auto *Lower = mdconst::dyn_extract<ConstantInt>(Range->getOperand(0));
    auto *Upper = mdconst::dyn_extract<ConstantInt>(Range->getOperand(1));
    if (Lower && Upper && Lower->getType() == IntTy &&
        Upper->getType() == IntTy) {
      APInt LowerValue = Lower->getValue();
      APInt UpperValue = Upper->getValue();
      if (LowerValue.ult(UpperValue)) {
        if (!Result) {
          Result = RangeBounds{LowerValue, UpperValue};
        } else {
          // Intersect: tighten the bounds from both sources.
          if (LowerValue.ugt(Result->Lower))
            Result->Lower = LowerValue;
          if (UpperValue.ult(Result->Upper))
            Result->Upper = UpperValue;
          if (Result->Lower.uge(Result->Upper))
            return std::nullopt;
        }
      }
    }
  }

  return Result;
}

bool setUnionRangeAnnotation(Instruction *To, ArrayRef<Value *> Values) {
  auto *CB = dyn_cast<CallBase>(To);
  if (!CB)
    return false;
  if (To->getMetadata(LLVMContext::MD_range) ||
      CB->getRetAttr(Attribute::Range).isValid())
    return false;

  std::optional<RangeBounds> Union;
  for (Value *V : Values) {
    std::optional<RangeBounds> Bounds = getSimpleRangeBounds(V, To->getType());
    if (!Bounds)
      return false;

    if (!Union) {
      Union = *Bounds;
      continue;
    }

    if (Bounds->Lower.ult(Union->Lower))
      Union->Lower = Bounds->Lower;
    if (Bounds->Upper.ugt(Union->Upper))
      Union->Upper = Bounds->Upper;
  }

  if (!Union)
    return false;

  MDBuilder MDB(To->getContext());
  MDNode *Range = MDB.createRange(Union->Lower, Union->Upper);
  if (!Range)
    return false;

  To->setMetadata(LLVMContext::MD_range, Range);
  CB->addRangeRetAttr(ConstantRange(Union->Lower, Union->Upper));
  return true;
}

bool copyRangeAnnotation(Instruction *To, Value *From) {
  auto *CB = dyn_cast<CallBase>(To);
  if (!CB)
    return false;
  if (To->getMetadata(LLVMContext::MD_range) ||
      CB->getRetAttr(Attribute::Range).isValid())
    return false;

  auto *FromI = dyn_cast<Instruction>(From);
  if (!FromI || FromI->getType() != To->getType())
    return false;

  std::optional<RangeBounds> Bounds =
      getSimpleRangeBounds(FromI, To->getType());
  if (!Bounds)
    return false;

  MDBuilder MDB(To->getContext());
  MDNode *Range = MDB.createRange(Bounds->Lower, Bounds->Upper);
  if (!Range)
    return false;

  To->setMetadata(LLVMContext::MD_range, Range);
  CB->addRangeRetAttr(ConstantRange(Bounds->Lower, Bounds->Upper));
  return true;
}

bool isZeroConstant(Value *V) {
  auto *C = dyn_cast<ConstantInt>(V);
  return C && C->isZero();
}

bool isWholeDimensionSlice(IntrinsicInst *Slice) {
  assert(Slice->getIntrinsicID() == Intrinsic::ripple_slice);
  for (unsigned Idx = 1, End = Slice->arg_size(); Idx != End; ++Idx) {
    auto *Arg = dyn_cast<ConstantInt>(Slice->getArgOperand(Idx));
    if (!Arg || !Arg->isNegative())
      return false;
  }
  return true;
}

} // namespace

PreservedAnalyses RippleAPIPreprocessPass::run(Function &F,
                                               FunctionAnalysisManager &AM) {
  DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  MemorySSA &MSSA = AM.getResult<MemorySSAAnalysis>(F).getMSSA();

  bool Changed = false;
  for (Instruction &I : make_early_inc_range(instructions(F))) {
    auto *II = dyn_cast<IntrinsicInst>(&I);
    if (!II)
      continue;

    Intrinsic::ID ID = II->getIntrinsicID();
    auto ReplaceIntrinsicWith = [&](Value *V) {
      auto It = II->getIterator();
      ReplaceInstWithValue(It, V);
      Changed = true;
    };

    if (ID == Intrinsic::ripple_slice && isWholeDimensionSlice(II)) {
      ReplaceIntrinsicWith(II->getArgOperand(0));
    } else if (ID == Intrinsic::ripple_broadcast) {
      // These API forms preserve the scalar value domain. Propagating !range
      // lets early scalar optimizers reason about the wrapper call result
      // directly.
      if (isZeroConstant(II->getArgOperand(1))) {
        ReplaceIntrinsicWith(II->getArgOperand(2));
      } else {
        Changed |= copyRangeAnnotation(II, II->getArgOperand(2));
      }
    } else if (ID == Intrinsic::ripple_reshape) {
      Changed |= copyRangeAnnotation(II, II->getArgOperand(1));
    } else if (ID == Intrinsic::ripple_slice) {
      Changed |= copyRangeAnnotation(II, II->getArgOperand(0));
    } else if (ID == Intrinsic::ripple_stack) {
      SmallVector<Value *, 8> Inputs;
      for (unsigned Idx = 1, End = II->arg_size(); Idx != End; ++Idx)
        Inputs.push_back(II->getArgOperand(Idx));
      Changed |= setUnionRangeAnnotation(II, Inputs);
    } else if (ID == Intrinsic::ripple_shuffle) {
      SmallVector<Value *, 2> Inputs;
      Inputs.push_back(II->getArgOperand(0));
      // A single-input shuffle ignores its second operand, which therefore does
      // not contribute to the range.
      if (!isZeroConstant(II->getArgOperand(2)))
        Inputs.push_back(II->getArgOperand(1));
      Changed |= setUnionRangeAnnotation(II, Inputs);
    } else if (isRippleReduction(ID)) {
      if (isZeroConstant(II->getArgOperand(0)))
        ReplaceIntrinsicWith(II->getArgOperand(1));
    } else if (ID == Intrinsic::ripple_block_getsize ||
               ID == Intrinsic::ripple_block_index) {
      std::optional<uint64_t> Dimension = getConstantOperandValue(II, 1);
      if (!Dimension)
        continue;

      RippleBlockShapeLookupResult Lookup =
          findRippleBlockShapeIntrinsic(II->getArgOperandUse(0), MSSA, DT,
                                        /*DetectAmbiguity=*/true);
      if (!Lookup)
        continue;

      std::optional<uint64_t> DimSize =
          getBlockShapeDimSize(Lookup.BlockShape, *Dimension);
      if (!DimSize)
        continue;

      if (ID == Intrinsic::ripple_block_getsize) {
        // Folding to a constant that the result type cannot represent would
        // silently truncate the size, so leave the call for Ripple to lower.
        auto *IntTy = dyn_cast<IntegerType>(II->getType());
        if (!IntTy || !isUIntN(IntTy->getBitWidth(), *DimSize))
          continue;
        ReplaceIntrinsicWith(ConstantInt::get(IntTy, *DimSize));
      } else if (ID == Intrinsic::ripple_block_index) {
        auto *IntTy = dyn_cast<IntegerType>(II->getType());
        if (!IntTy)
          continue;
        if (*DimSize == 1)
          ReplaceIntrinsicWith(ConstantInt::get(IntTy, 0));
        else
          Changed |= setUnsignedRangeAnnotation(II, *DimSize);
      } else {
        llvm_unreachable("unexpected intrinsic in Ripple API preprocess pass");
      }
    }
  }

  if (!Changed)
    return PreservedAnalyses::all();

  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}
