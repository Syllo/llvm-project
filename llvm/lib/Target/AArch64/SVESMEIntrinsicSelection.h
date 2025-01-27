//===- SVESMEIntrinsicSelection.h -----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AARCH64_SVE_SME_INTRINSIC_SELECTION_H
#define LLVM_LIB_TARGET_AARCH64_SVE_SME_INTRINSIC_SELECTION_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class SVESMEIntrinsicSelectionPass
    : public PassInfoMixin<SVESMEIntrinsicSelectionPass> {
public:
  explicit SVESMEIntrinsicSelectionPass() {}
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_AARCH64_SVE_SME_INTRINSIC_SELECTION_H
