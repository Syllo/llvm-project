//===--------- RippleModulePass.cpp - Expand Ripple intrinsics ------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This pass expands Ripple intrinsics.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Ripple/RippleModulePass.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/ADT/ilist_iterator.h"
#include "llvm/ADT/iterator.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/InlineCost.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueHandle.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Ripple/Preprocess/RippleFPExtFPTrunc.h"
#include "llvm/Transforms/Ripple/Preprocess/RippleFPExtFPTruncRevert.h"
#include "llvm/Transforms/Ripple/Preprocess/RippleSESE.h"
#include "llvm/Transforms/Ripple/Ripple.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace llvm;

#define DEBUG_TYPE "ripple"

llvm::cl::opt<bool> OnlySESE("ripple-run-only-sese", llvm::cl::init(false),
                             cl::ReallyHidden,
                             llvm::cl::desc("Run ripple SESE only (testing)"));
llvm::cl::opt<bool> OnlyRipple(
    "ripple-run-only-ripple-passes", llvm::cl::init(false), cl::ReallyHidden,
    llvm::cl::desc("Run ripple passes only, no post-processing (testing)"));

PreservedAnalyses RippleModulePass::run(Module &M, ModuleAnalysisManager &MAM) {
  FunctionAnalysisManager &FAM =
      MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

  Ripple::ProcessingStatus PS;
  DenseSet<AssertingVH<Function>> SpecializationsPendingProcessing;
  DenseSet<AssertingVH<Function>> SpecializationsAvailable;

  FunctionPassManager FPM;
  FPM.addPass(RippleSESEPass(TM, PS, SpecializationsPendingProcessing,
                             SpecializationsAvailable));
  if (!OnlySESE)
    FPM.addPass(RipplePass(TM, PS, SpecializationsPendingProcessing,
                           SpecializationsAvailable));

  PreservedAnalyses PA = PreservedAnalyses::all();

  auto runRipplePassOnFunction = [&](Function *F) -> void {
    LLVM_DEBUG(dbgs() << "Running ripple module pass on " << F->getName()
                      << "\n");
    // PassInstrumentation callbacks are driven by FPM for each contained pass.
    PreservedAnalyses PassPA = FPM.run(*F, FAM);

    // We know that the function pass couldn't have invalidated any other
    // function's analyses (that's the contract of a function pass), so
    // directly handle the function analysis manager's invalidation here.
    FAM.invalidate(*F, PassPA);

    // Then intersect the preserved set so that invalidation of module
    // analyses will eventually occur when the module pass completes.
    PA.intersect(std::move(PassPA));
  };

  FunctionPassManager PreProcessPasses;
  PreProcessPasses.addPass(RippleFPExtFPTruncRevertPass());
  for (auto &F : M) {
    if (!F.isDeclaration()) {
      // PassInstrumentation callbacks are driven by PreProcessPasses for each
      // contained pass.
      PreservedAnalyses PassPA = PreProcessPasses.run(F, FAM);
      FAM.invalidate(F, PassPA);
      PA.intersect(std::move(PassPA));
    }
  }

  DenseSet<AssertingVH<Function>> AlreadyProcessed;

  CallGraph CG(M);
  ReversePostOrderTraversal<const CallGraph *> RPOT(&CG);
  // Processing in reverse post-order as well as processing specialization
  // first is required:
  // - to specialize we need to clone a function un-processed by ripple (to
  // preserve the ripple constructs that may affect the return shape)
  // - specialization may request specializations through function call so we
  // need to clone non-specialized functions.

  // There is another approach that I though about later:
  // clone all the non-specialization functions being processed by ripple so
  // that we have the original at hand when needed
  // We continue until we processed all specializations and functions

  // The number of specialization requests is finite because of the broadcast
  // rule of shape propagation: you can only bcast or reduce the tensor shape
  // starting from constants
  bool SpecializationProgress;
  // When we encounter an error, we process all the functions at least once so
  // that we report as many errors as possible at once
  bool EncounteredErrors = false;
  while (true) {

    // We are progressing if we process some specialization or "normal
    // functions"
    SpecializationProgress = SpecializationsPendingProcessing.empty();
    {
      SmallVector<Function *> ToProcess(
          SpecializationsPendingProcessing.begin(),
          SpecializationsPendingProcessing.end());
      for (Function *F : ToProcess) {
        unsigned NumberPendingBefore = SpecializationsPendingProcessing.size();
        runRipplePassOnFunction(F);
        bool HasNewSpecialization =
            SpecializationsPendingProcessing.size() > NumberPendingBefore;

        if (PS == Ripple::ProcessingStatus::Success) {
          SpecializationsPendingProcessing.erase(F);
          F->eraseFromParent();
          SpecializationProgress = true;
        } else if (PS == Ripple::ProcessingStatus::ShapePropagationFailure ||
                   PS == Ripple::ProcessingStatus::SemanticsCheckFailure)
          EncounteredErrors = true;

        // Insertions invalidate the iterator so we need to re-enter
        if (HasNewSpecialization) {
          SpecializationProgress = true;
          break;
        }
      }
    }
    // Break when we are not progressing (specialization cycle) but continue on
    // Errors so that we print as many errors as possible before exiting the
    // pass
    if (!SpecializationProgress && !EncounteredErrors)
      break;

    // We process all specializations first. The reason is that specializations
    // may request other specializations and we need to clone the original
    // functions before processing them.
    if (!SpecializationsPendingProcessing.empty() && !EncounteredErrors)
      continue;

    // Process functions in the module in a caller->callee order (reverse post
    // order on the call graph). This is required so that we don't process a
    // callee before being able to clone it to create a ripple specialization it
    // for the caller.
    for (auto *CGN : RPOT) {
      Function *F = CGN->getFunction();
      if (F && !F->isDeclaration() && !AlreadyProcessed.contains(F)) {
        runRipplePassOnFunction(F);
        // Either the function was processed successfully
        if (PS == Ripple::ProcessingStatus::Success) {
          AlreadyProcessed.insert(F);
        } else if (PS == Ripple::ProcessingStatus::WaitingForSpecialization) {
          // Or the function has requested a specialization that we need to
          // process first, before the other functions in RPOT
          assert(!SpecializationsPendingProcessing.empty());
          if (!EncounteredErrors)
            break;
        } else if (PS == Ripple::ProcessingStatus::ShapePropagationFailure ||
                   PS == Ripple::ProcessingStatus::SemanticsCheckFailure) {
          // Or there was an error, we continue processing to report as many
          // errors as possible
          EncounteredErrors = true;
        }
      }
    }

    // We are done processing on errors or when everyone was processed!
    if (EncounteredErrors || SpecializationsPendingProcessing.empty())
      break;
  }

  // Cycle error reporting
  if (!EncounteredErrors && !SpecializationProgress) {
    LLVM_DEBUG(dbgs() << "Checking for cycles!\n");
    for (auto F : SpecializationsPendingProcessing) {
      SmallPtrSet<Function *, 8> VisitedFunctions;
      // Check for cycles staring with the specialization
      std::function<bool(const CallGraphNode *)> visitRecursively;
      visitRecursively = [&](const CallGraphNode *CGN) -> bool {
        LLVM_DEBUG(dbgs() << "Visiting " << CGN->getFunction()->getName()
                          << "\n");
        if (VisitedFunctions.contains(CGN->getFunction()))
          return true;
        // Find the base function of this specialization
        VisitedFunctions.insert(CGN->getFunction());

        for (auto &[_, Callee] : *CGN) {
          if (Callee)
            if (visitRecursively(Callee))
              return true;
        }

        VisitedFunctions.erase(CGN->getFunction());

        return false;
      };
      Function *OriginalFunction = Ripple::specializationOriginalFunction(*F);
      assert(OriginalFunction);
      if (visitRecursively(CG[OriginalFunction])) {
        // We found a cycle
        std::string ErrorMsg;
        {
          raw_string_ostream ErrStream(ErrorMsg);
          ErrStream
              << "Ripple encountered a cycle with "
                 "the following functions and cannot reliably propagate tensor "
                 "shapes in this case:";
          // Get consistent error messages
          std::vector<Function *> FVector(VisitedFunctions.begin(),
                                          VisitedFunctions.end());
          std::sort(FVector.begin(), FVector.end(),
                    [](Function *L, Function *R) {
                      return L->getName() < R->getName();
                    });
          for (size_t Idx = 0, E = FVector.size(); Idx < E; ++Idx) {
            if (Idx)
              ErrStream << ",";
            ErrStream << " " << FVector[Idx]->getName();
          }
        }
        M.getContext().diagnose(DiagnosticInfoGeneric(
            nullptr, ErrorMsg, DiagnosticSeverity::DS_Error));
        return PreservedAnalyses::none();
      }
    }
    M.getContext().diagnose(
        DiagnosticInfoGeneric(nullptr,
                              "Ripple entered an non-progress state "
                              "due to specialization cycles",
                              DiagnosticSeverity::DS_Error));
    return PreservedAnalyses::none();
  }

  // Inline functions marked with ripple-vector-candidate.
  //
  // We do this here, inside the ripple module pass, rather than relying on the
  // standard inliner pipeline. It allows us to restrict the work to
  // ripple-vector-candidate calls. It also keeps this step cheap: we walk the
  // module once, only enqueue calls whose callee carries the attribute, and
  // never touch unrelated call sites.
  {
    SmallVector<WeakTrackingVH, 16> CallsToInline;
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;
      for (Instruction &I : instructions(F)) {
        if (auto *CB = dyn_cast<CallBase>(&I)) {
          if (Function *Callee = CB->getCalledFunction()) {
            if (Callee->hasFnAttribute("ripple-vector-candidate") &&
                !Callee->isDeclaration()) {
              CallsToInline.push_back(CB);
            }
          }
        }
      }
    }

    if (!CallsToInline.empty()) {
      auto GetAssumptionCache = [&](Function &F) -> AssumptionCache & {
        return FAM.getResult<AssumptionAnalysis>(F);
      };
      // Only request PSI when the module actually carries profile metadata.
      ProfileSummaryInfo *PSI = M.getProfileSummary(/*IsCS=*/false)
                                    ? &MAM.getResult<ProfileSummaryAnalysis>(M)
                                    : nullptr;
      auto GetBFI = [&](Function &F) -> BlockFrequencyInfo & {
        return FAM.getResult<BlockFrequencyAnalysis>(F);
      };
      auto GetTLI = [&](Function &F) -> const TargetLibraryInfo & {
        return FAM.getResult<TargetLibraryAnalysis>(F);
      };

      // Track already-inlined (caller, callee) pairs to break cycles between
      // mutually-recursive ripple-vector-candidate functions.
      DenseSet<std::pair<Function *, Function *>> InlinedPairs;

      while (!CallsToInline.empty()) {
        auto *CB = dyn_cast_or_null<CallBase>(CallsToInline.pop_back_val());
        if (!CB)
          continue;
        Function *Callee = CB->getCalledFunction();
        // We only ever enqueue calls whose callee is a defined ripple-vector
        // candidate, and nothing in this loop rewrites call targets or strips
        // the attribute, so these invariants must still hold at pop time.
        assert(Callee && Callee->hasFnAttribute("ripple-vector-candidate") &&
               !Callee->isDeclaration() &&
               "queued call site lost its candidate target");

        Function *Caller = CB->getCaller();

        auto &CalleeTTI = FAM.getResult<TargetIRAnalysis>(*Callee);
        auto OIC = llvm::getInlineCost(*CB, Callee, llvm::getInlineParams(),
                                       CalleeTTI, GetAssumptionCache, GetTLI,
                                       GetBFI, PSI, nullptr, nullptr);
        bool ShouldInline = static_cast<bool>(OIC);

        if (ShouldInline) {
          InlineFunctionInfo IFI;
          if (InlineFunction(*CB, IFI).isSuccess()) {
            FAM.invalidate(*Caller, PreservedAnalyses::none());
            PA.intersect(PreservedAnalyses::none());
            InlinedPairs.insert({Caller, Callee});

            for (CallBase *InlinedCB : IFI.InlinedCallSites) {
              if (Function *NewCallee = InlinedCB->getCalledFunction()) {
                if (NewCallee->hasFnAttribute("ripple-vector-candidate") &&
                    !NewCallee->isDeclaration() &&
                    !InlinedPairs.contains({Caller, NewCallee})) {
                  CallsToInline.push_back(InlinedCB);
                }
              }
            }
          } else {
            auto &ORE =
                FAM.getResult<OptimizationRemarkEmitterAnalysis>(*Caller);
            ORE.emit([&] {
              OptimizationRemarkMissed R(DEBUG_TYPE, "CandidateInlineFailed",
                                         CB);
              R << "could not inline ripple-vector-candidate '"
                << ore::NV("Callee", Callee) << "' into '"
                << ore::NV("Caller", Caller) << "'";
              return R;
            });
          }
        } else {
          // The cost model declined to inline this candidate. Surface a
          // ripple-namespaced remark so it appears under
          // -pass-remarks-missed=ripple (the cost model's own remarks would
          // land under "inline-cost" instead).
          auto &ORE =
              FAM.getResult<OptimizationRemarkEmitterAnalysis>(*Caller);
          ORE.emit([&] {
            OptimizationRemarkMissed R(DEBUG_TYPE, "CandidateNotInlined", CB);
            R << "did not inline ripple-vector-candidate '"
              << ore::NV("Callee", Callee) << "' into '"
              << ore::NV("Caller", Caller) << "'";
            if (const char *Reason = OIC.getReason())
              R << ": " << Reason;
            return R;
          });
        }
      }
    }
  }

  FunctionPassManager PostProcessPasses;
  PostProcessPasses.addPass(RippleFPExtFPTruncPass());
  if (!OnlyRipple) {
    PostProcessPasses.addPass(InstCombinePass());
  }
  for (auto &F : M) {
    Ripple::eraseFunctionSpecializationRelatedMetadata(F);
    if (!F.isDeclaration()) {
      // PassInstrumentation callbacks are driven by PostProcessPasses for each
      // contained pass.
      PreservedAnalyses PassPA = PostProcessPasses.run(F, FAM);
      FAM.invalidate(F, PassPA);
      PA.intersect(std::move(PassPA));
    }
  }

  // Drop ripple-vector-candidate functions from llvm.used / llvm.compiler.used.
  // Clang appended them there so they would survive pre-ripple GlobalDCE; now
  // that the ripple pass has run, candidates that were actually substituted
  // remain alive through their call references, and unused ones become
  // collectible.
  SmallPtrSet<GlobalValue *, 16> RippleCandidates;
  for (Function &F : M)
    if (F.hasFnAttribute("ripple-vector-candidate"))
      RippleCandidates.insert(&F);
  if (!RippleCandidates.empty()) {
    removeFromUsedLists(M, [&](Constant *C) {
      auto *GV = dyn_cast<GlobalValue>(C->stripPointerCasts());
      return GV && RippleCandidates.contains(GV);
    });
    // removeFromUsedLists updates @llvm.used / @llvm.compiler.used. Function
    // bodies are untouched, so keep function analyses preserved.
    PreservedAnalyses FunctionAnalysesOnly;
    FunctionAnalysesOnly.preserveSet<AllAnalysesOn<Function>>();
    PA.intersect(FunctionAnalysesOnly);
  }

  return PA;
}
