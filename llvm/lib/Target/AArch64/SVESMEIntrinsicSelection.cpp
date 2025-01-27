//===- SVESMEIntrinsicSelectionPass ---------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Experimental prototype to transform Ripple output fixed-length wide vector IR
// to AArch64 target SVE SME intrinsics.
//
//===----------------------------------------------------------------------===//

#include "SVESMEIntrinsicSelection.h"
#include "AArch64.h"
#include "Utils/AArch64BaseInfo.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IntrinsicsAArch64.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/IR/Verifier.h"
#include "llvm/InitializePasses.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;
using namespace llvm::PatternMatch;

#define DEBUG_TYPE "aarch64-sve-sme-intrinsic-selection"

extern cl::opt<bool> EnableSVESMEIntrinsicSelection;

namespace {
class SVESMEIntrinsicSelectionImpl {
  friend class SVESMEIntrinsicSelection;

  enum GEPIndexingType {
    Tiled,
    NonTiled,
    UnsupportedIndexingType,
  };

public:
  SVESMEIntrinsicSelectionImpl(LoopInfo &LI, Function &F)
      : LI(LI), F(F), Builder(F.getContext()) {}

  // Rewrite fixed-length vector IR into SVE SME intrinsic calls.
  bool transform(bool &AbortIRRewrite);

private:
  unsigned NeonVectorRegisterSize = 128;

  unsigned FixedVScale = 0;

  LoopInfo &LI;

  Function &F;

  IRBuilder<> Builder;

  SmallMapVector<Instruction *, bool, 4> OuterProductValidityMap;

  bool
  extractPartialTileBoundsFromVectorPredicate(Value *Predicate,
                                              Value *&PartialTileLowerBound,
                                              Value *&PartialTileUpperBound);

  bool extractPredicates(Value *PredVector, Value *&PredA0, Value *&PredA1,
                         Value *&PredB0, Value *&PredB1, bool &IsAllTruePredA,
                         bool &IsAllTruePredB, Type *ElemTy,
                         BasicBlock *Preheader);

  bool extractTileOffset(Value *PointerVector, Value *&CBasePtr,
                         Value *&TileOffset, Value *&NDim,
                         BasicBlock *Preheader, BasicBlock *StoreBB);

  bool expandStoreInst(Instruction *SI,
                       SmallSetVector<Instruction *, 16> &DeadInsts);

  bool selectOuterProductInst(Instruction *MulAdd,
                              SmallSetVector<Instruction *, 16> &DeadInsts);

  bool isValidOuterProduct(Instruction *MulAdd);
};

class SVESMEIntrinsicSelection : public ModulePass {
public:
  static char ID;

  SVESMEIntrinsicSelection() : ModulePass(ID) {
    initializeSVESMEIntrinsicSelectionPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override {
    return "SVE SME Intrinsic Selection Pass";
  }

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<DominatorTreeWrapperPass>();
  }
};

} // namespace

char SVESMEIntrinsicSelection::ID = 0;

bool SVESMEIntrinsicSelection::runOnModule(Module &M) {
  if (!EnableSVESMEIntrinsicSelection)
    return false;

  LLVM_DEBUG(dbgs() << getPassName() << "\n");

  // Mapping functions with partial IR rewrite to its original function.
  SmallMapVector<Function *, Function *, 2> FunctionsToRestore;
  SmallSetVector<Function *, 2> ClonedFunctions;
  bool Changed = false;
  for (auto &F : M.getFunctionList()) {
    if (F.isDeclaration())
      continue;
    if (ClonedFunctions.count(&F))
      continue;

    // Restrict SVE SME intrinsic rewrite to functions with streaming mode
    // locally enabled and a new ZA matrix declared, and operating at fixed
    // streaming mode vector bits.
    if (!(F.hasFnAttribute("aarch64_pstate_sm_body") &&
          F.hasFnAttribute("aarch64_new_za"))) {
      LLVM_DEBUG(dbgs() << "Function not set with attribute "
                           "__arm_locally_streaming and __arm_new(\"za\")\n");
      continue;
    }
    auto Attr = F.getFnAttribute(Attribute::VScaleRange);
    if (!Attr.isValid()) {
      LLVM_DEBUG(dbgs() << "No valid VScaleRange\n");
      continue;
    }
    unsigned MinVScale = Attr.getVScaleRangeMin();
    std::optional<unsigned> MaxVScale = Attr.getVScaleRangeMax();
    if (!MaxVScale || MinVScale != MaxVScale) {
      LLVM_DEBUG(dbgs() << "No MaxVScale or VScale not set as fixed value\n");
      continue;
    }

    auto &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
    SVESMEIntrinsicSelectionImpl Impl(LI, F);

    ValueToValueMapTy VMap;
    Function *OrigFunc = CloneFunction(&F, VMap);
    bool AbortIRRewrite = false;
    bool LocallyChanged = Impl.transform(AbortIRRewrite);
    Changed |= LocallyChanged;
    if (!LocallyChanged || AbortIRRewrite) {
      DiagnosticInfoOptimizationFailure Diag(
          F, {},
          "Failed to transform " + F.getName() +
              " for SME optimization. Consider optimization using "
              "non-streaming mode SVE or NEON.");
      F.getContext().diagnose(Diag);
    }
    if (AbortIRRewrite) {
      FunctionsToRestore.insert({&F, OrigFunc});
      ClonedFunctions.insert(OrigFunc);
    } else
      OrigFunc->eraseFromParent();
  }

  for (auto FuncPair : FunctionsToRestore) {
    FuncPair.second->takeName(FuncPair.first);
    FuncPair.first->replaceAllUsesWith(FuncPair.second);
    FuncPair.first->eraseFromParent();
  }

  return Changed;
}

INITIALIZE_PASS_BEGIN(
    SVESMEIntrinsicSelection, DEBUG_TYPE,
    "Lower fixed-length wide vector to SVE SME target specific intrinsics",
    false, false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
INITIALIZE_PASS_END(
    SVESMEIntrinsicSelection, DEBUG_TYPE,
    "Lower fixed-length wide vector to SVE SME target specific intrinsics",
    false, false)

ModulePass *llvm::createSVESMEIntrinsicSelectionPass() {
  return new SVESMEIntrinsicSelection();
}

bool SVESMEIntrinsicSelectionImpl::transform(bool &AbortIRRewrite) {
  FixedVScale = F.getFnAttribute(Attribute::VScaleRange).getVScaleRangeMin();
  LLVM_DEBUG(
      dbgs() << "SVE SME Intrinsic rewrite at streaming mode fixed vscale "
             << FixedVScale << "\n");

  // TODO: Bail out if encounter invalid block size.
  // E.g., for floating point type block size should be 32x32.

  SmallSetVector<Instruction *, 16> DeadInsts;
  bool Changed = false;
  for (auto &I : instructions(F)) {
    if (isa<IntrinsicInst>(&I) &&
        cast<IntrinsicInst>(&I)->getIntrinsicID() == Intrinsic::fmuladd) {
      if (!isa<VectorType>(I.getType()))
        continue;
      if (!isValidOuterProduct(&I))
        continue;

      bool LocallyChanged = selectOuterProductInst(&I, DeadInsts);
      Changed |= LocallyChanged;
      // Abort IR rewrite if fail to transform fmuladd intrinsic into SME mopa
      // intrinsics. So that store expansion will be reverted or not triggered.
      AbortIRRewrite = !LocallyChanged;
      if (AbortIRRewrite)
        break;
    }

    if (isa<StoreInst>(&I) ||
        (isa<IntrinsicInst>(&I) && cast<IntrinsicInst>(&I)->getIntrinsicID() ==
                                       Intrinsic::masked_scatter)) {
      Type *StoreValueTy =
          isa<StoreInst>(&I)
              ? cast<StoreInst>(&I)->getValueOperand()->getType()
              : cast<IntrinsicInst>(&I)->getOperand(0)->getType();
      if (!isa<VectorType>(StoreValueTy))
        continue;

      if (!OuterProductValidityMap.contains(&I)) {
        Instruction *MulAdd = dyn_cast<Instruction>(I.getOperand(0));
        if (!MulAdd || !isValidOuterProduct(MulAdd))
          continue;
      }

      if (OuterProductValidityMap[&I]) {
        bool LocallyChanged = expandStoreInst(&I, DeadInsts);
        Changed |= LocallyChanged;
        // Abort IR rewrite if fail to expand store instruction. So that outer
        // product selection on fmuladd instruction will be reverted or not
        // triggered.
        AbortIRRewrite = !LocallyChanged;
        if (AbortIRRewrite)
          break;
      }
    }
  }
  if (!Changed || AbortIRRewrite)
    return false;

  for (auto *I : DeadInsts)
    I->eraseFromParent();
  LLVM_DEBUG(dbgs() << "After SVE SME Intrinsic rewrite\n"; F.dump());

  verifyFunction(F);

  return true;
}

static bool isHorizontalShuffle(Value *Shuffle) {
  Value *V0;
  ArrayRef<int> Mask;
  unsigned ShuffleVectorLength;
  if (match(Shuffle, m_Shuffle(m_Value(V0), m_Poison(), m_Mask(Mask)))) {
    ShuffleVectorLength =
        cast<FixedVectorType>(V0->getType())->getNumElements();
    if (ShuffleVectorLength * ShuffleVectorLength != Mask.size())
      return false;

    for (unsigned i = 0, Idx = 0; i < Mask.size();
         i += ShuffleVectorLength, Idx++)
      for (unsigned j = 0; j < ShuffleVectorLength; j++)
        if ((unsigned)Mask[i + j] != Idx)
          return false;

    return true;
  }

  return false;
}

static bool isVerticalShuffle(Value *Shuffle) {
  Value *V0;
  ArrayRef<int> Mask;
  unsigned ShuffleVectorLength;
  if (match(Shuffle, m_Shuffle(m_Value(V0), m_Poison(), m_Mask(Mask)))) {
    ShuffleVectorLength =
        cast<FixedVectorType>(V0->getType())->getNumElements();
    if (ShuffleVectorLength * ShuffleVectorLength != Mask.size())
      return false;

    for (unsigned i = 0; i < Mask.size(); i += ShuffleVectorLength)
      for (unsigned j = 0; j < ShuffleVectorLength; j++)
        if ((unsigned)Mask[i + j] != j)
          return false;

    return true;
  }

  return false;
}

bool SVESMEIntrinsicSelectionImpl::isValidOuterProduct(Instruction *MulAdd) {
  IntrinsicInst *II = dyn_cast<IntrinsicInst>(MulAdd);
  if (!II || (II->getIntrinsicID() != Intrinsic::fmuladd)) {
    LLVM_DEBUG(
        dbgs()
        << "Expect outer product to be a fmuladd intrinsic instruction\n");
    return false;
  }

  if (OuterProductValidityMap.contains(MulAdd))
    return OuterProductValidityMap[MulAdd];

  Instruction *SI = nullptr;
  for (auto U : MulAdd->users()) {
    if (auto *UI = dyn_cast<Instruction>(U))
      if (isa<StoreInst>(UI) || (isa<IntrinsicInst>(UI) &&
                                 cast<IntrinsicInst>(UI)->getIntrinsicID() ==
                                     Intrinsic::masked_scatter))
        SI = UI;
  }
  // Outer product fmuladd needs to pair with a store.
  if (!SI) {
    OuterProductValidityMap.insert({MulAdd, false});
    return false;
  }
  assert(!OuterProductValidityMap.contains(SI) &&
         "Don't expect OuterProductValidityMap to have entry for store\n");

  // Make sure fmuladd is multiplying a horizontal splat with a vertical splat.
  bool IsValidShufflePair = (isHorizontalShuffle(II->getOperand(0)) &&
                             isVerticalShuffle(II->getOperand(1))) ||
                            (isHorizontalShuffle(II->getOperand(1)) &&
                             isVerticalShuffle(II->getOperand(0)));
  if (!IsValidShufflePair) {
    OuterProductValidityMap.insert({MulAdd, false});
    OuterProductValidityMap.insert({SI, false});
    return false;
  }

  PHINode *AddAccumulator = dyn_cast<PHINode>(II->getOperand(2));
  if (!AddAccumulator || AddAccumulator->getNumIncomingValues() != 2) {
    OuterProductValidityMap.insert({MulAdd, false});
    OuterProductValidityMap.insert({SI, false});
    return false;
  }

  Value *InitVal = AddAccumulator->getIncomingValue(0) == MulAdd
                       ? AddAccumulator->getIncomingValue(1)
                       : AddAccumulator->getIncomingValue(0);

  bool IsZeroInitVal =
      isa<Constant>(InitVal) && cast<Constant>(InitVal)->isZeroValue();

  OuterProductValidityMap.insert({MulAdd, IsZeroInitVal});
  OuterProductValidityMap.insert({SI, IsZeroInitVal});

  // Make sure add accumulator initial value is zero.
  return IsZeroInitVal;
}

bool SVESMEIntrinsicSelectionImpl::selectOuterProductInst(
    Instruction *MulAdd, SmallSetVector<Instruction *, 16> &DeadInsts) {
  LLVM_DEBUG(dbgs() << "Select outer product Instruction " << *MulAdd << "\n");

  Type *ElemTy = cast<VectorType>(MulAdd->getType())->getElementType();
  // TODO: Expand support for other data types.
  if (!ElemTy->isFloatTy())
    return false;

  LLVMContext &Ctx = F.getContext();
  Type *Int32Ty = Type::getInt32Ty(Ctx);
  Type *Int64Ty = Type::getInt64Ty(Ctx);
  unsigned FixedVectorNumElems =
      NeonVectorRegisterSize / ElemTy->getScalarSizeInBits();
  Type *NewLoadType = ScalableVectorType::get(ElemTy, FixedVectorNumElems);

  // TODO: Handle special case when K equals 1, there is no loop over
  // outerproduct.
  Loop *L = LI.getLoopFor(MulAdd->getParent());
  if (!L)
    return false;

  BasicBlock *Preheader = L->getLoopPreheader();
  assert(Preheader && "Expect loop preheader\n");
  Builder.SetInsertPoint(Preheader->getTerminator());
  // Zero ZA matrix.
  Builder.CreateIntrinsic(Intrinsic::aarch64_sme_zero, {},
                          {ConstantInt::get(Int32Ty, 255)});

  Builder.SetInsertPoint(MulAdd);
  Instruction *HorizontalShuffle = cast<Instruction>(
      isHorizontalShuffle(MulAdd->getOperand(0)) ? MulAdd->getOperand(0)
                                                 : MulAdd->getOperand(1));
  Instruction *VerticalShuffle = cast<Instruction>(
      isVerticalShuffle(MulAdd->getOperand(1)) ? MulAdd->getOperand(1)
                                               : MulAdd->getOperand(0));
  Instruction *A0 = cast<Instruction>(HorizontalShuffle->getOperand(0));
  assert((isa<LoadInst>(A0) ||
          match(A0, m_Intrinsic<Intrinsic::masked_load>()) ||
          match(A0, m_Intrinsic<Intrinsic::masked_gather>())) &&
         "Expect A0 being load inst or masked.gather/load intrinsic "
         "instruction\n");
  Instruction *B0 = cast<Instruction>(VerticalShuffle->getOperand(0));
  assert((isa<LoadInst>(B0) ||
          match(B0, m_Intrinsic<Intrinsic::masked_load>()) ||
          match(B0, m_Intrinsic<Intrinsic::masked_gather>())) &&
         "Expect B0 being load inst or masked.gather/load intrinsic "
         "instruction\n");

  auto splitFixedVectorLoadIntoTwoScalableVectors = [&](Instruction *Load,
                                                        Value *&ScalableVec0,
                                                        Value *&ScalableVec1) {
    assert((FixedVectorNumElems * FixedVScale ==
            (cast<FixedVectorType>(Load->getType())->getNumElements() / 2)) &&
           "Expect fixed-length vector load to contain twice the amount of "
           "elements from a scalable vector load\n");
    Type *HalfFixedLengthLoadType =
        FixedVectorType::get(ElemTy, FixedVectorNumElems * FixedVScale);
    Value *FixedVec0 = Builder.CreateIntrinsic(
        HalfFixedLengthLoadType, Intrinsic::vector_extract,
        {Load, ConstantInt::get(Int64Ty, 0)});
    ScalableVec0 =
        Builder.CreateIntrinsic(NewLoadType, Intrinsic::vector_insert,
                                {PoisonValue::get(NewLoadType), FixedVec0,
                                 ConstantInt::get(Int64Ty, 0)});
    Value *FixedVec1 = Builder.CreateIntrinsic(
        HalfFixedLengthLoadType, Intrinsic::vector_extract,
        {Load, ConstantInt::get(Int64Ty, FixedVectorNumElems * FixedVScale)});
    ScalableVec1 =
        Builder.CreateIntrinsic(NewLoadType, Intrinsic::vector_insert,
                                {PoisonValue::get(NewLoadType), FixedVec1,
                                 ConstantInt::get(Int64Ty, 0)});
  };
  Value *NewA0 = nullptr, *NewA1 = nullptr, *NewB0 = nullptr, *NewB1 = nullptr;
  splitFixedVectorLoadIntoTwoScalableVectors(A0, NewA0, NewA1);
  splitFixedVectorLoadIntoTwoScalableVectors(B0, NewB0, NewB1);

  // Create outerproduct instrinsic calls to use four ZA tiles.
  // For floating point type, there are four ZA tiles available.
  Value *AllTrueSplat = Builder.CreateVectorSplat(
      ElementCount::getScalable(FixedVectorNumElems), Builder.getTrue());

  Value *Args0[] = {ConstantInt::get(Int32Ty, 0), AllTrueSplat, AllTrueSplat,
                    NewA0, NewB0};
  Value *OuterProduct0 =
      Builder.CreateIntrinsic(Intrinsic::aarch64_sme_mopa, NewLoadType, Args0);
  LLVM_DEBUG(dbgs() << "OuterProduct tile 0 created " << *OuterProduct0
                    << "\n");

  Value *Args1[] = {ConstantInt::get(Int32Ty, 1), AllTrueSplat, AllTrueSplat,
                    NewA0, NewB1};
  Value *OuterProduct1 =
      Builder.CreateIntrinsic(Intrinsic::aarch64_sme_mopa, NewLoadType, Args1);
  LLVM_DEBUG(dbgs() << "OuterProduct tile 1 created " << *OuterProduct1
                    << "\n");
  Value *Args2[] = {ConstantInt::get(Int32Ty, 2), AllTrueSplat, AllTrueSplat,
                    NewA1, NewB0};
  Value *OuterProduct2 =
      Builder.CreateIntrinsic(Intrinsic::aarch64_sme_mopa, NewLoadType, Args2);
  LLVM_DEBUG(dbgs() << "OuterProduct tile 2 created " << *OuterProduct2
                    << "\n");
  Value *Args3[] = {ConstantInt::get(Int32Ty, 3), AllTrueSplat, AllTrueSplat,
                    NewA1, NewB1};
  Value *OuterProduct3 =
      Builder.CreateIntrinsic(Intrinsic::aarch64_sme_mopa, NewLoadType, Args3);
  LLVM_DEBUG(dbgs() << "OuterProduct tile 3 created " << *OuterProduct3
                    << "\n");

  // TODO: Check if there are better ways to erase dead Instructions.
  DeadInsts.insert(MulAdd);
  for (User *U : MulAdd->users())
    // TODO: If there are other input values for masked.scatter, split out
    // original store basic block, and preserve that input value for
    // masked.scatter.
    if (Instruction *UI = dyn_cast<Instruction>(U))
      DeadInsts.insert(UI);
  DeadInsts.insert(cast<Instruction>(MulAdd->getOperand(0)));
  DeadInsts.insert(cast<Instruction>(MulAdd->getOperand(1)));
  MulAdd->replaceAllUsesWith(UndefValue::get(MulAdd->getType()));

  return true;
}

static bool isStepOneIncrementVector(Constant *Const) {
  ConstantDataVector *CDV = dyn_cast<ConstantDataVector>(Const);
  if (!CDV)
    return false;

  for (unsigned i = 0, e = CDV->getNumElements(); i < e; i++) {
    if (CDV->getElementAsInteger(i) != i)
      return false;
  }
  return true;
}

static bool isVerticalSplatOfStepOneIncrementVector(Constant *Const) {
  ConstantDataVector *CDV = dyn_cast<ConstantDataVector>(Const);
  if (!CDV)
    return false;

  unsigned ShuffleVectorLength =
      *APInt(64, CDV->getNumElements()).sqrt().getRawData();
  for (unsigned i = 0, e = CDV->getNumElements(), idx = 0; i < e; i++, idx++) {
    idx = (idx == ShuffleVectorLength ? 0 : idx);
    if (CDV->getElementAsInteger(i) != idx)
      return false;
  }
  return true;
}

static Value *getSplatValue(ShuffleVectorInst *V) {
  assert(isa<InsertElementInst>(V->getOperand(0)) &&
         "Expect to get splat value from InsertElementInst\n");
  return cast<Instruction>(V->getOperand(0))->getOperand(1);
}

static bool isIdentityShuffle(Value *V) {
  ShuffleVectorInst *Shuffle = dyn_cast<ShuffleVectorInst>(V);
  if (!Shuffle)
    return false;

  if (Shuffle->isZeroEltSplat() &&
      match(Shuffle->getOperand(0),
            m_InsertElt(m_Poison(), m_Value(), m_Zero())))
    return true;

  return false;
}

static bool isValidSingleDimensionTileOffset(Value *V,
                                             Value *UpperBound = nullptr) {
  if (isa<CastInst>(V))
    V = cast<CastInst>(V)->getOperand(0);

  // Matching phi [0, predecessor0], ['ii*32' or 'jj*32', predecessor1]. ii and
  // jj are tile indexes at i and j dimension, 32 is the block size.
  Value *ShlV;
  ConstantInt *ShlAmount;
  if (match(V, m_Shl(m_Value(ShlV), m_ConstantInt(ShlAmount))) &&
      isa<PHINode>(ShlV)) {
    PHINode *Phi = cast<PHINode>(ShlV);
    if (Phi->getNumIncomingValues() != 2)
      return false;
    Value *PhiV0 = Phi->getIncomingValue(0);
    Value *PhiV1 = Phi->getIncomingValue(1);
    if ((match(PhiV0, m_Zero()) &&
         match(PhiV1, m_c_Add(m_Specific(Phi), m_One()))) ||
        (match(PhiV1, m_Zero()) &&
         match(PhiV0, m_c_Add(m_Specific(Phi), m_One()))))
      return true;
  }

  Value *Div;
  if (PHINode *Phi = dyn_cast<PHINode>(V)) {
    if (Phi->getNumIncomingValues() != 2)
      return false;
    Value *PhiV0 = Phi->getIncomingValue(0);
    Value *PhiV1 = Phi->getIncomingValue(1);
    if ((match(PhiV0, m_Zero()) &&
         match(PhiV1, m_Shl(m_Value(Div), m_ConstantInt(ShlAmount)))) ||
        (match(PhiV1, m_Zero()) &&
         match(PhiV0, m_Shl(m_Value(Div), m_ConstantInt(ShlAmount))))) {
      ConstantInt *DivAmount;
      if ((UpperBound
               ? match(Div,
                       m_SDiv(m_Specific(UpperBound), m_ConstantInt(DivAmount)))
               : match(Div, m_SDiv(m_Value(), m_ConstantInt(DivAmount)))) &&
          DivAmount->getValue().exactLogBase2() == ShlAmount->getSExtValue())
        return true;
    }
  }

  return false;
}

static bool isIntegerScalarType(Value *V) {
  if (!isa<Instruction>(V) && V->getType()->isIntegerTy())
    return true;

  return false;
}

// Check if a vector predicate is coming from a compare instruction that is
// built solely on tile boundary index and loop upper bound, and extract
// partial tile bounds.
bool SVESMEIntrinsicSelectionImpl::extractPartialTileBoundsFromVectorPredicate(
    Value *Predicate, Value *&PartialTileLowerBound,
    Value *&PartialTileUpperBound) {
  CmpPredicate CmpPred;
  Constant *Const;
  Value *CmpLHS, *CmpRHS;
  if (match(Predicate, m_ICmp(CmpPred, m_Value(CmpLHS), m_Value(CmpRHS))) &&
      CmpPred == ICmpInst::ICMP_SLT && isIdentityShuffle(CmpRHS)) {
    PartialTileUpperBound = getSplatValue(cast<ShuffleVectorInst>(CmpRHS));
    if (!isIntegerScalarType(PartialTileUpperBound))
      return false;

    if (isVerticalShuffle(CmpLHS))
      CmpLHS = cast<ShuffleVectorInst>(CmpLHS)->getOperand(0);
    if (match(CmpLHS,
              m_c_Or(m_Value(PartialTileLowerBound), m_Constant(Const))) &&
        isStepOneIncrementVector(Const) &&
        isIdentityShuffle(PartialTileLowerBound)) {
      PartialTileLowerBound =
          getSplatValue(cast<ShuffleVectorInst>(PartialTileLowerBound));
      // Assuming loop tiling using block size 32, matching partial tile lower
      // bound: phi [0, pred0], [(PartialTileUpperBound/32)<<log2(32), pred1].
      if (isValidSingleDimensionTileOffset(PartialTileLowerBound,
                                           PartialTileUpperBound))
        return true;
    }
  }

  return false;
}

bool SVESMEIntrinsicSelectionImpl::extractPredicates(
    Value *PredVector, Value *&PredA0, Value *&PredA1, Value *&PredB0,
    Value *&PredB1, bool &IsAllTruePredA, bool &IsAllTruePredB, Type *ElemTy,
    BasicBlock *Preheader) {
  LLVM_DEBUG(dbgs() << "Extract predicates from " << *PredVector << "\n");

  Builder.SetInsertPoint(Preheader);
  unsigned FixedVectorNumElems =
      NeonVectorRegisterSize / ElemTy->getScalarSizeInBits();
  ScalableVectorType *PredTy =
      ScalableVectorType::get(Builder.getInt1Ty(), FixedVectorNumElems);
  Type *HalfFixedLengthPredTy = FixedVectorType::get(
      Builder.getInt1Ty(), FixedVectorNumElems * FixedVScale);
  Type *Int64Ty = Type::getInt64Ty(F.getContext());

  // Tile with all true predicate on A and B.
  if (ConstantVector *ConstVec = dyn_cast<ConstantVector>(PredVector)) {
    ConstantInt *CInt =
        dyn_cast_or_null<ConstantInt>(ConstVec->getSplatValue());
    if (CInt && CInt->isOne()) {
      Value *AllTrueSplat = Builder.CreateVectorSplat(
          ElementCount::getScalable(FixedVectorNumElems), Builder.getTrue());
      PredA0 = PredA1 = PredB0 = PredB1 = AllTrueSplat;
      IsAllTruePredA = IsAllTruePredB = true;
      return true;
    }
  }

  auto buildSVEPredicatesFromScalarBounds = [&](Value *Base, Value *UpperBound,
                                                Value *&Pred0, Value *&Pred1) {
    UpperBound = UpperBound->getType() == Builder.getInt64Ty()
                     ? UpperBound
                     : Builder.CreateSExt(UpperBound, Builder.getInt64Ty());
    Base = Base->getType() == Builder.getInt64Ty()
               ? Base
               : Builder.CreateSExt(Base, Builder.getInt64Ty());
    Value *SecondBase = Builder.CreateAdd(
        Base, Builder.getInt64(FixedVScale * FixedVectorNumElems));
    Pred0 = Builder.CreateIntrinsic(Intrinsic::aarch64_sve_whilelt,
                                    {PredTy, Builder.getInt64Ty()},
                                    {Base, UpperBound});
    Pred1 = Builder.CreateIntrinsic(Intrinsic::aarch64_sve_whilelt,
                                    {PredTy, Builder.getInt64Ty()},
                                    {SecondBase, UpperBound});
    LLVM_DEBUG(dbgs() << "Create Pred0 " << *Pred0 << "\n");
    LLVM_DEBUG(dbgs() << "Create Pred1 " << *Pred1 << "\n");
  };

  auto splitFixedPredicateIntoTwoScalablePredicates =
      [&](Value *FixedPred, Value *&ScalablePred0, Value *&ScalablePred1) {
        assert((FixedVectorNumElems * FixedVScale ==
                (cast<FixedVectorType>(FixedPred->getType())->getNumElements() /
                 2)) &&
               "Expect fixed-length predicate vector to contain twice the "
               "amount of elements from a scalable predicate vector\n");
        Value *FixedHalfPredVec0 = Builder.CreateIntrinsic(
            HalfFixedLengthPredTy, Intrinsic::vector_extract,
            {FixedPred, ConstantInt::get(Int64Ty, 0)});
        ScalablePred0 = Builder.CreateIntrinsic(
            PredTy, Intrinsic::vector_insert,
            {PoisonValue::get(PredTy), FixedHalfPredVec0,
             ConstantInt::get(Int64Ty, 0)});
        Value *FixedHalfPredVec1 = Builder.CreateIntrinsic(
            HalfFixedLengthPredTy, Intrinsic::vector_extract,
            {FixedPred,
             ConstantInt::get(Int64Ty, FixedVectorNumElems * FixedVScale)});
        ScalablePred1 = Builder.CreateIntrinsic(
            PredTy, Intrinsic::vector_insert,
            {PoisonValue::get(PredTy), FixedHalfPredVec1,
             ConstantInt::get(Int64Ty, 0)});
      };

  Value *PartialTileLowerBound = nullptr, *PartialTileUpperBound = nullptr;
  // Tile with predicate on B, all true predicate on A.
  if (isVerticalShuffle(PredVector)) {
    IsAllTruePredA = true;

    Value *PredVec = cast<ShuffleVectorInst>(PredVector)->getOperand(0);
    // TODO: Use SVE predicates to replace masked.load/gather predicates, if
    // masked load and store are using the same predicate.
    if (extractPartialTileBoundsFromVectorPredicate(
            PredVec, PartialTileLowerBound, PartialTileUpperBound))
      buildSVEPredicatesFromScalarBounds(PartialTileLowerBound,
                                         PartialTileUpperBound, PredB0, PredB1);
    else
      splitFixedPredicateIntoTwoScalablePredicates(PredVec, PredB0, PredB1);

    return true;
  }

  // Tile with predicate on A, all true predicate on B.
  if (isHorizontalShuffle(PredVector)) {
    IsAllTruePredB = true;

    Value *PredVec = cast<ShuffleVectorInst>(PredVector)->getOperand(0);
    if (extractPartialTileBoundsFromVectorPredicate(
            PredVec, PartialTileLowerBound, PartialTileUpperBound))
      buildSVEPredicatesFromScalarBounds(PartialTileLowerBound,
                                         PartialTileUpperBound, PredA0, PredA1);
    else
      splitFixedPredicateIntoTwoScalablePredicates(PredVec, PredA0, PredA1);

    Value *AllTrueSplatInt8 = Builder.CreateVectorSplat(
        ElementCount::getScalable(NeonVectorRegisterSize / 8),
        Builder.getTrue());
    PredB0 = PredB1 = AllTrueSplatInt8;

    return true;
  }

  // Tile with predicate on A and B.
  if (isa<BinaryOperator>(PredVector) &&
      cast<BinaryOperator>(PredVector)->getOpcode() == BinaryOperator::And) {
    Value *Op0 = cast<BinaryOperator>(PredVector)->getOperand(0);
    Value *Op1 = cast<BinaryOperator>(PredVector)->getOperand(1);
    Value *HorizontalShuffle = isHorizontalShuffle(Op0)
                                   ? Op0
                                   : (isHorizontalShuffle(Op1) ? Op1 : nullptr);
    Value *VerticalShuffle =
        isVerticalShuffle(Op0) ? Op0 : (isVerticalShuffle(Op1) ? Op1 : nullptr);
    assert(
        HorizontalShuffle && VerticalShuffle &&
        "Expect binary operation 'and' on horizontal and vertical shuffle\n");

    Value *PredVec = cast<ShuffleVectorInst>(VerticalShuffle)->getOperand(0);
    if (extractPartialTileBoundsFromVectorPredicate(
            PredVec, PartialTileLowerBound, PartialTileUpperBound))
      buildSVEPredicatesFromScalarBounds(PartialTileLowerBound,
                                         PartialTileUpperBound, PredB0, PredB1);
    else
      splitFixedPredicateIntoTwoScalablePredicates(PredVec, PredB0, PredB1);
    PredB0 = Builder.CreateIntrinsic(Intrinsic::aarch64_sve_convert_to_svbool,
                                     {PredTy}, {PredB0});
    PredB1 = Builder.CreateIntrinsic(Intrinsic::aarch64_sve_convert_to_svbool,
                                     {PredTy}, {PredB1});

    PredVec = cast<ShuffleVectorInst>(HorizontalShuffle)->getOperand(0);
    if (extractPartialTileBoundsFromVectorPredicate(
            PredVec, PartialTileLowerBound, PartialTileUpperBound))
      buildSVEPredicatesFromScalarBounds(PartialTileLowerBound,
                                         PartialTileUpperBound, PredA0, PredA1);
    else
      splitFixedPredicateIntoTwoScalablePredicates(PredVec, PredA0, PredA1);

    return true;
  }

  return false;
}

bool SVESMEIntrinsicSelectionImpl::extractTileOffset(
    Value *PointerVector, Value *&CBasePtr, Value *&TileOffset, Value *&NDim,
    BasicBlock *Preheader, BasicBlock *OriginalStoreBB) {
  GEPOperator *GEP = dyn_cast<GEPOperator>(PointerVector);
  if (!GEP || GEP->getNumIndices() != 1) {
    LLVM_DEBUG(
        dbgs()
        << "Expect to extract tile offset based on one gep offset indice\n");
    return false;
  }

  LLVM_DEBUG(dbgs() << "Extract tile offset on pointer vector "
                    << *PointerVector << "\n");

  // Assuming block size is 32, then each processing element is indexed at:
  // C + (ii*32+i)*N + jj*32 + j. This function extract tile offset
  // ii*32*N + jj*32, in the meanwhile, strip out invariant gep i*N+j. Then tile
  // base can be re-constructed by adding tile offset to base pointer C.

  Value *BasePtrs = GEP->getPointerOperand();
  Value *Offset = GEP->getOperand(1);

  // Step 1: Extract tile offset.
  // Strip out vertical splat for '+j', as it's part of invariant gep.
  if (isVerticalShuffle(Offset))
    Offset = cast<ShuffleVectorInst>(Offset)->getOperand(0);

  // Strip out type cast.
  if (isa<CastInst>(Offset))
    Offset = cast<CastInst>(Offset)->getOperand(0);

  Value *V0;
  Constant *Const;
  // Strip out '+j'.
  if (match(Offset, m_c_Or(m_Value(V0), m_Constant(Const))) &&
      isStepOneIncrementVector(Const))
    Offset = V0;

  // Get shuffle identity.
  if (ShuffleVectorInst *Shuffle = dyn_cast<ShuffleVectorInst>(Offset))
    if (isIdentityShuffle(Shuffle))
      Offset = getSplatValue(cast<ShuffleVectorInst>(Shuffle));

  // Pattern matching for phi [0,predecessor0], [jj*32, predecessor1].
  if (PHINode *Phi = dyn_cast<PHINode>(Offset)) {
    if (Phi->getNumIncomingValues() == 2) {
      Value *In0 = Phi->getIncomingValue(0);
      Value *In1 = Phi->getIncomingValue(1);
      ConstantInt *ShlAmount;
      if (ConstantInt *CInt = dyn_cast<ConstantInt>(In0)) {
        if (CInt->isZero() &&
            match(In1, m_Shl(m_Value(), m_ConstantInt(ShlAmount))))
          // TODO: Check that ShlAmount == 5 (log2 of blocksize).
          TileOffset = Offset;
      }
      if (ConstantInt *CInt = dyn_cast<ConstantInt>(In1)) {
        if (CInt->isZero() &&
            match(In0, m_Shl(m_Value(), m_ConstantInt(ShlAmount))))
          // TODO: Check that ShlAmount == 5 (log2 of blocksize).
          TileOffset = Offset;
      }
    }
  }

  if (isa<BinaryOperator>(Offset) &&
      cast<BinaryOperator>(Offset)->getOpcode() == BinaryOperator::Add) {
    Value *Op0 = cast<BinaryOperator>(Offset)->getOperand(0);
    Value *Op1 = cast<BinaryOperator>(Offset)->getOperand(1);

    // Handling Op0.
    Value *V0, *V1;
    Constant *Const;
    if (match(Op0, m_c_Add(m_Value(Offset), m_Constant(Const))) &&
        isVerticalSplatOfStepOneIncrementVector(Const)) {
      // Strip out '+j'.
      if (isHorizontalShuffle(Offset))
        Offset = cast<ShuffleVectorInst>(Offset)->getOperand(0);

      if (match(Offset, m_c_Mul(m_Value(V0), m_Shuffle(m_Value(V1), m_Poison(),
                                                       m_ZeroMask()))) &&
          match(V1, m_InsertElt(m_Poison(), m_Value(NDim), m_Zero()))) {
        // Strip out '+i'.
        if (match(V0, m_c_Or(m_Value(V1), m_Constant(Const))) &&
            isStepOneIncrementVector(Const)) {

          if (isIdentityShuffle(V1))
            Offset = getSplatValue(cast<ShuffleVectorInst>(V1));
          if (!isValidSingleDimensionTileOffset(Offset))
            return false;

          Builder.SetInsertPoint(Preheader);
          Offset = Builder.CreateMul(Offset, NDim);
          if (TileOffset && TileOffset->getType() != Offset->getType()) {
            TileOffset =
                TileOffset->getType() == Builder.getInt64Ty()
                    ? TileOffset
                    : Builder.CreateSExt(TileOffset, Builder.getInt64Ty());
            Offset = Offset->getType() == Builder.getInt64Ty()
                         ? Offset
                         : Builder.CreateSExt(Offset, Builder.getInt64Ty());
          }
          TileOffset =
              TileOffset ? Builder.CreateAdd(Offset, TileOffset) : Offset;
        }
      }
    }
    if (isVerticalShuffle(Op0)) {
      // Strip out '+j'.
      Op0 = cast<ShuffleVectorInst>(Op0)->getOperand(0);

      if (match(Op0, m_c_Or(m_Value(V0), m_Constant(Const))) &&
          isStepOneIncrementVector(Const))
        Op0 = V0;

      if (isIdentityShuffle(Op0))
        Op0 = getSplatValue(cast<ShuffleVectorInst>(Op0));

      if (isValidSingleDimensionTileOffset(Op0)) {
        assert(!TileOffset && "Do not expect TileOffset to have value.\n");
        TileOffset = Op0;
      }
    }

    // Handling Op1.
    if (isIdentityShuffle(Op1)) {
      Op1 = getSplatValue(cast<ShuffleVectorInst>(Op1));

      if (isa<CastInst>(Op1))
        Op1 = cast<CastInst>(Op1)->getOperand(0);

      // After recognizing jj*32, add that into ii*32*N.
      if (TileOffset) {
        Builder.SetInsertPoint(Preheader);
        if (TileOffset->getType() != Op1->getType()) {
          TileOffset =
              TileOffset->getType() == Builder.getInt64Ty()
                  ? TileOffset
                  : Builder.CreateSExt(TileOffset, Builder.getInt64Ty());
          Op1 = Op1->getType() == Builder.getInt64Ty()
                    ? Op1
                    : Builder.CreateSExt(Op1, Builder.getInt64Ty());
        }
        TileOffset = Builder.CreateAdd(TileOffset, Op1);
      }
    }
    if (isHorizontalShuffle(Op1)) {
      Op1 = cast<ShuffleVectorInst>(Op1)->getOperand(0);
      Value *V0, *V1, *Temp;
      if (match(Op1, m_c_Mul(m_Value(V0), m_Shuffle(m_Value(V1), m_Poison(),
                                                    m_ZeroMask()))) &&
          match(V1, m_InsertElt(m_Poison(), m_Value(NDim), m_Zero()))) {
        // Strip out '+i' from (ii*32+i)*N.
        if (match(V0, m_c_Or(m_Value(V1), m_Constant(Const))) &&
            isStepOneIncrementVector(Const) && isIdentityShuffle(V1)) {
          Temp = getSplatValue(cast<ShuffleVectorInst>(V1));

          if (isValidSingleDimensionTileOffset(Temp)) {
            Builder.SetInsertPoint(Preheader);
            Temp = Builder.CreateMul(Temp, NDim);
            // Check if type cast is needed for add.
            // Note that type cast is not needed for mul.
            TileOffset =
                TileOffset ? Builder.CreateAdd(Temp, TileOffset) : Temp;
          }
        }
      }
    }
  }

  // Step 2: Extract C base pointer.
  // Extract CBasePtr if BasePtrs is an identity splat.
  if (isIdentityShuffle(BasePtrs)) {
    CBasePtr = getSplatValue(cast<ShuffleVectorInst>(BasePtrs));
    return true;
  }

  if (isHorizontalShuffle(BasePtrs))
    BasePtrs = cast<ShuffleVectorInst>(BasePtrs)->getOperand(0);

  // TODO: Generalize gep handling with more than one index.
  GEP = dyn_cast<GEPOperator>(BasePtrs);
  if (!GEP && BasePtrs->getType()->isPointerTy() && NDim && TileOffset) {
    CBasePtr = BasePtrs;
    return true;
  }

  if (!GEP || GEP->getNumIndices() != 1) {
    LLVM_DEBUG(
        dbgs() << "Only able to extract tile.base based on one gep index\n");
    return false;
  }

  Offset = GEP->getOperand(1);
  if (isa<BinaryOperator>(Offset) &&
      cast<BinaryOperator>(Offset)->getOpcode() == BinaryOperator::Mul) {
    Value *Op0 = cast<BinaryOperator>(Offset)->getOperand(0);
    Value *Op1 = cast<BinaryOperator>(Offset)->getOperand(1);
    NDim = isValidSingleDimensionTileOffset(Op0)
               ? Op1
               : (isValidSingleDimensionTileOffset(Op1) ? Op0 : nullptr);
    if (!NDim)
      return false;

    Builder.SetInsertPoint(Preheader);
    if (TileOffset && TileOffset->getType() != Offset->getType())
      TileOffset = Builder.CreateSExt(TileOffset, Offset->getType());
    if (cast<Instruction>(Offset)->getParent() == OriginalStoreBB) {
      Instruction *OffsetClone = cast<Instruction>(Offset)->clone();
      OffsetClone->insertAfter(&Preheader->front());
      Offset = OffsetClone;
    }
    TileOffset = TileOffset ? Builder.CreateAdd(Offset, TileOffset) : Offset;
  }

  // TODO: Strip out type based multiply. E.g., float can be indexing based on
  // i8 type, then type based multiply of 4 need to be stripped out.
  BasePtrs = GEP->getPointerOperand();
  if (isa<GEPOperator>(BasePtrs) && cast<GEPOperator>(BasePtrs)
                                        ->getPointerOperand()
                                        ->getType()
                                        ->isPointerTy()) {
    CBasePtr = cast<GEPOperator>(BasePtrs)->getPointerOperand();
  }

  return true;
}

bool SVESMEIntrinsicSelectionImpl::expandStoreInst(
    Instruction *SI, SmallSetVector<Instruction *, 16> &DeadInsts) {
  LLVM_DEBUG(dbgs() << "Expand masked store intrinsic " << *SI << "\n");

  GEPOperator *GEP = dyn_cast<GEPOperator>(SI->getOperand(1));
  if (!GEP) {
    LLVM_DEBUG(dbgs() << "Store pointer operand not a GEPOperator\n");
    return false;
  }
  GEPIndexingType GEPTy = UnsupportedIndexingType;
  if (GEP->getNumIndices() == 1)
    GEPTy = NonTiled;
  else if (GEP->getNumIndices() == 4) {
    ConstantInt *Indice3 = dyn_cast<ConstantInt>(GEP->getOperand(3));
    ConstantInt *Indice4 = dyn_cast<ConstantInt>(GEP->getOperand(4));
    if (Indice3 && Indice3->isZero() && Indice4 && Indice4->isZero())
      GEPTy = Tiled;
  }
  if (GEPTy == UnsupportedIndexingType) {
    LLVM_DEBUG(dbgs() << "Unsupported GEP indexing type\n");
    return false;
  }

  bool IsAllTruePredA = false, IsAllTruePredB = false;
  if (isa<StoreInst>(SI))
    IsAllTruePredA = IsAllTruePredB = true;
  else {
    IntrinsicInst *II = dyn_cast<IntrinsicInst>(SI);
    assert((II && II->getIntrinsicID() == Intrinsic::masked_scatter) &&
           "Expect masked_scatter intrinsic instruction");
    if (Constant *ConstVal = dyn_cast<Constant>(II->getOperand(3)))
      if (ConstVal->isAllOnesValue())
        IsAllTruePredA = IsAllTruePredB = true;
  }

  LLVMContext &Ctx = F.getContext();
  BasicBlock *StoreBB = SI->getParent();
  Type *Int32Ty = Type::getInt32Ty(Ctx);
  Type *ElemTy =
      cast<VectorType>(SI->getOperand(0)->getType())->getElementType();
  unsigned FixedVectorNumElems =
      NeonVectorRegisterSize / ElemTy->getScalarSizeInBits();

  // Build loop structure to expand store of one block into a loop of storing
  // vector tile slices.
  BasicBlock *LoopBody = BasicBlock::Create(Ctx, "store.body", &F, StoreBB);
  BasicBlock *Preheader =
      BasicBlock::Create(Ctx, "store.preheader", &F, LoopBody);
  BranchInst *BI =
      cast<BranchInst>(StoreBB->getSinglePredecessor()->getTerminator());
  if (BI->getSuccessor(0) == StoreBB)
    BI->setSuccessor(0, Preheader);
  else
    BI->setSuccessor(1, Preheader);

  Value *Offset = GEP->getOperand(1);
  if (isa<CastInst>(Offset))
    Offset = cast<CastInst>(Offset)->getOperand(0);

  Value *CBasePtr = nullptr, *TileOffset = nullptr, *NDim = nullptr,
        *TileBase = nullptr;
  Value *PredA0 = nullptr, *PredA1 = nullptr, *PredB0 = nullptr,
        *PredB1 = nullptr;
  if (GEPTy == NonTiled) {
    if (!extractTileOffset(GEP, CBasePtr, TileOffset, NDim, Preheader, StoreBB))
      return false;
    assert(TileOffset && CBasePtr && NDim &&
           "Expect to get CBasePtr, TileOffset and NDim information\n");

    // If GEP indexing is non-tiled, then expect store intruction to be
    // masked.scatter intrinsic.
    if (!extractPredicates(SI->getOperand(3), PredA0, PredA1, PredB0, PredB1,
                           IsAllTruePredA, IsAllTruePredB, ElemTy, Preheader))
      return false;
  }

  // Construct store preheader.
  Builder.SetInsertPoint(Preheader);
  if (GEPTy == NonTiled) {
    TileBase = Builder.CreateGEP(ElemTy, CBasePtr, TileOffset, "tile.base");
    LLVM_DEBUG(dbgs() << "Re-construct tile base " << *TileBase << "\n");
    if (!NDim->getType()->isIntegerTy(64))
      NDim = Builder.CreateSExt(NDim, Builder.getInt64Ty(), "N.ext");
  }

  Value *VScale = Builder.CreateIntrinsic(Intrinsic::vscale, {Int32Ty}, {},
                                          /*FMFSource=*/nullptr, "vscale");
  Value *ScalableVectorLength = Builder.CreateMul(
      VScale, ConstantInt::get(Int32Ty, FixedVectorNumElems), "veclen");
  Builder.CreateBr(LoopBody);

  // Construct store loop body.
  Builder.SetInsertPoint(LoopBody);
  PHINode *Indvar = Builder.CreatePHI(Type::getInt64Ty(Ctx), 2);
  Indvar->addIncoming(ConstantInt::get(Indvar->getType(), 0), Preheader);
  PHINode *Slice = Builder.CreatePHI(Int32Ty, 2);
  Slice->addIncoming(ConstantInt::get(Slice->getType(), 0), Preheader);
  Value *PtrPHI = nullptr;
  if (GEPTy == NonTiled) {
    PtrPHI = Builder.CreatePHI(
        cast<VectorType>(SI->getOperand(1)->getType())->getElementType(), 2);
    cast<PHINode>(PtrPHI)->addIncoming(TileBase, Preheader);
  }

  // Load ZA matrix in Int8 type, so that two of the tile slices are contiguous
  // in memory.
  Value *AllTrueSplatInt8 = Builder.CreateVectorSplat(
      ElementCount::getScalable(NeonVectorRegisterSize / 8), Builder.getTrue());
  Type *ReadHorType = ScalableVectorType::get(Type::getInt8Ty(Ctx), 16);
  Type *StoreType = ScalableVectorType::get(ElemTy, FixedVectorNumElems);
  Value *Zero = ConstantInt::get(Int32Ty, 0);
  Value *Args0[] = {UndefValue::get(ReadHorType), AllTrueSplatInt8, Zero,
                    Slice};
  Value *ReadHor0 = Builder.CreateIntrinsic(Intrinsic::aarch64_sme_read_horiz,
                                            ReadHorType, Args0);
  ReadHor0 = Builder.CreateBitCast(ReadHor0, StoreType);
  LLVM_DEBUG(dbgs() << "Horizontal read 0 created " << *ReadHor0 << "\n");
  Value *Slice1 = Builder.CreateAdd(Slice, ConstantInt::get(Int32Ty, 1));
  Value *Args1[] = {UndefValue::get(ReadHorType), AllTrueSplatInt8, Zero,
                    Slice1};
  Value *ReadHor1 = Builder.CreateIntrinsic(Intrinsic::aarch64_sme_read_horiz,
                                            ReadHorType, Args1);
  ReadHor1 = Builder.CreateBitCast(ReadHor1, StoreType);
  LLVM_DEBUG(dbgs() << "Horizontal read 1 created " << *ReadHor1 << "\n");
  Value *Slice2 = Builder.CreateAdd(Slice, ConstantInt::get(Int32Ty, 2));
  Value *Args2[] = {UndefValue::get(ReadHorType), AllTrueSplatInt8, Zero,
                    Slice2};
  Value *ReadHor2 = Builder.CreateIntrinsic(Intrinsic::aarch64_sme_read_horiz,
                                            ReadHorType, Args2);
  ReadHor2 = Builder.CreateBitCast(ReadHor2, StoreType);
  LLVM_DEBUG(dbgs() << "Horizontal read 2 created " << *ReadHor2 << "\n");
  Value *Slice3 = Builder.CreateAdd(Slice, ConstantInt::get(Int32Ty, 3));
  Value *Args3[] = {UndefValue::get(ReadHorType), AllTrueSplatInt8, Zero,
                    Slice3};
  Value *ReadHor3 = Builder.CreateIntrinsic(Intrinsic::aarch64_sme_read_horiz,
                                            ReadHorType, Args3);
  ReadHor3 = Builder.CreateBitCast(ReadHor3, StoreType);
  LLVM_DEBUG(dbgs() << "Horizontal read 3 created " << *ReadHor3 << "\n");

  // Construct predicate selection.
  Value *StorePred0 = nullptr, *StorePred1 = nullptr, *StorePred2 = nullptr,
        *StorePred3 = nullptr;
  Value *AllTrueSplat = Builder.CreateVectorSplat(
      ElementCount::getScalable(FixedVectorNumElems), Builder.getTrue());
  if (IsAllTruePredA && IsAllTruePredB) {
    StorePred0 = StorePred1 = StorePred2 = StorePred3 = AllTrueSplat;
  } else if (IsAllTruePredA && !IsAllTruePredB) {
    StorePred0 = StorePred2 = PredB0;
    StorePred1 = StorePred3 = PredB1;
  } else { /*!IsAllTruePredA*/
    Value *IVTrunc = Builder.CreateTrunc(Indvar, Builder.getInt32Ty());
    if (IsAllTruePredB) {
      ScalableVectorType *Ty =
          ScalableVectorType::get(Builder.getInt1Ty(), FixedVectorNumElems);
      Value *PSelA0B01 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB0, PredA0, IVTrunc});
      PSelA0B01 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA0B01});
      StorePred0 = StorePred1 = PSelA0B01;

      Value *PSelA1B01 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB0, PredA1, IVTrunc});
      PSelA1B01 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA1B01});
      StorePred2 = StorePred3 = PSelA1B01;
    } else { /*!IsAllTruePredA && !IsAllTruePredB*/
      ScalableVectorType *Ty =
          ScalableVectorType::get(Builder.getInt1Ty(), FixedVectorNumElems);
      Value *PSelA0B0 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB0, PredA0, IVTrunc});
      StorePred0 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA0B0});

      Value *PSelA0B1 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB1, PredA0, IVTrunc});
      StorePred1 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA0B1});

      Value *PSelA1B0 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB0, PredA1, IVTrunc});
      StorePred2 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA1B0});

      Value *PSelA1B1 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_psel, {Ty}, {PredB1, PredA1, IVTrunc});
      StorePred3 = Builder.CreateIntrinsic(
          Intrinsic::aarch64_sve_convert_from_svbool, {Ty}, {PSelA1B1});
    }
  }
  assert(StorePred0 && StorePred1 && StorePred2 && StorePred3 &&
         "Expect to get four store predicates\n");

  if (GEPTy == Tiled) {
    PtrPHI =
        cast<Instruction>(cast<StoreInst>(SI)->getPointerOperand())->clone();
    cast<Instruction>(PtrPHI)->setOperand(3, Indvar);
    cast<Instruction>(PtrPHI)->insertAfter(cast<Instruction>(ReadHor3));
  }

  Align ElemAlign = Align(ElemTy->getScalarSizeInBits() / 8);
  Value *NewStore0 =
      Builder.CreateMaskedStore(ReadHor0, PtrPHI, ElemAlign, StorePred0);
  LLVM_DEBUG(dbgs() << "New store 0 created " << *NewStore0 << "\n");
  Value *Gep1 = Builder.CreateGEP(ElemTy, PtrPHI, ScalableVectorLength);
  Value *NewStore1 =
      Builder.CreateMaskedStore(ReadHor1, Gep1, ElemAlign, StorePred1);
  LLVM_DEBUG(dbgs() << "New store 1 created " << *NewStore1 << "\n");
  Value *Gep2 = nullptr;
  if (GEPTy == NonTiled) {
    Value *SecondStoreGroupOffset = Builder.CreateMul(
        NDim, ConstantInt::get(Type::getInt64Ty(Ctx),
                               FixedVectorNumElems * FixedVScale));
    Gep2 = Builder.CreateGEP(ElemTy, PtrPHI, SecondStoreGroupOffset);
  } else if (GEPTy == Tiled) {
    Value *Offset =
        Builder.CreateAdd(Indvar, Builder.CreateZExt(ScalableVectorLength,
                                                     Type::getInt64Ty(Ctx)));
    Gep2 = cast<Instruction>(cast<StoreInst>(SI)->getPointerOperand())->clone();
    cast<Instruction>(Gep2)->setOperand(3, Offset);
    cast<Instruction>(Gep2)->insertAfter(cast<Instruction>(Offset));
  }
  Value *NewStore2 =
      Builder.CreateMaskedStore(ReadHor2, Gep2, ElemAlign, StorePred2);
  LLVM_DEBUG(dbgs() << "New store 2 created " << *NewStore2 << "\n");
  Value *Gep3 = Builder.CreateGEP(ElemTy, Gep2, ScalableVectorLength);
  Value *NewStore3 =
      Builder.CreateMaskedStore(ReadHor3, Gep3, ElemAlign, StorePred3);
  LLVM_DEBUG(dbgs() << "New store 3 created " << *NewStore3 << "\n");

  Value *UpdatedIndvar =
      Builder.CreateAdd(Indvar, ConstantInt::get(Indvar->getType(), 1));
  Indvar->addIncoming(UpdatedIndvar, LoopBody);
  Value *UpdatedSlice = Builder.CreateAdd(
      Slice, ConstantInt::get(Slice->getType(), FixedVectorNumElems));
  Slice->addIncoming(UpdatedSlice, LoopBody);
  if (GEPTy == NonTiled) {
    Value *UpdatedPtr = Builder.CreateGEP(ElemTy, PtrPHI, NDim);
    cast<PHINode>(PtrPHI)->addIncoming(UpdatedPtr, LoopBody);
  }
  Value *ExitCond = Builder.CreateICmp(
      ICmpInst::ICMP_EQ, UpdatedIndvar,
      ConstantInt::get(Indvar->getType(), FixedVectorNumElems * FixedVScale));
  Builder.CreateCondBr(ExitCond, StoreBB, LoopBody);

  DeadInsts.insert(SI);
  DeadInsts.insert(cast<Instruction>(SI->getOperand(1)));

  return true;
}
