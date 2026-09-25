//===- TensorShapeTests.cpp - -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RippleTestBase.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicsRipple.h"
#include "llvm/IR/Module.h"
#include "llvm/Transforms/Ripple/Ripple.h"
#include "gtest/gtest.h"

namespace llvm {
namespace {

using TensorShapeTest = RippleTestBase;
using TensorShapeAttributeTest = RippleFunctionTest;

TEST_F(TensorShapeTest, Accessors) {
  // Test default constructor (scalar)
  TensorShape scalar;
  EXPECT_EQ(scalar.rank(), 0u);
  EXPECT_TRUE(scalar.isScalar());

  // Test constructor with rank and default value
  TensorShape shape1(3);
  EXPECT_EQ(shape1.rank(), 3u);
  EXPECT_EQ(shape1[0], 1u);
  EXPECT_EQ(shape1[1], 1u);
  EXPECT_EQ(shape1[2], 1u);

  // Test constructor with rank and specific value
  TensorShape shape2(3, 5);
  EXPECT_EQ(shape2.rank(), 3u);
  EXPECT_EQ(shape2[0], 5u);
  EXPECT_EQ(shape2[1], 5u);
  EXPECT_EQ(shape2[2], 5u);

  // Test constructor from array
  std::vector<uint64_t> dims = {2, 3, 4};
  ArrayRef<uint64_t> dimsRef(dims);
  TensorShape shape3(dimsRef);
  EXPECT_EQ(shape3.rank(), 3u);
  EXPECT_EQ(shape3[0], 2u);
  EXPECT_EQ(shape3[1], 3u);
  EXPECT_EQ(shape3[2], 4u);

  // Test constructor with rank, dimension, and size
  TensorShape shape4(4, 2, 10);
  EXPECT_EQ(shape4.rank(), 4u);
  EXPECT_EQ(shape4[0], 1u);
  EXPECT_EQ(shape4[1], 1u);
  EXPECT_EQ(shape4[2], 10u);
  EXPECT_EQ(shape4[3], 1u);

  // Test operator[] with out-of-bounds index (should return 1)
  EXPECT_EQ(shape3[10], 1u);

  // Test getShape()
  const auto &shapeVec = shape3.getShape();
  EXPECT_EQ(shapeVec.size(), 3u);
  EXPECT_EQ(shapeVec[0], 2u);
  EXPECT_EQ(shapeVec[1], 3u);
  EXPECT_EQ(shapeVec[2], 4u);

  // Test forward iteration with begin()/end()
  std::vector<uint64_t> collected;
  for (auto it = shape3.begin(); it != shape3.end(); ++it) {
    collected.push_back(*it);
  }
  EXPECT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 2u);
  EXPECT_EQ(collected[1], 3u);
  EXPECT_EQ(collected[2], 4u);

  // Test range-based for loop
  collected.clear();
  for (auto dim : shape3) {
    collected.push_back(dim);
  }
  EXPECT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 2u);
  EXPECT_EQ(collected[1], 3u);
  EXPECT_EQ(collected[2], 4u);

  // Test reverse iteration with rbegin()/rend()
  collected.clear();
  for (auto it = shape3.rbegin(); it != shape3.rend(); ++it) {
    collected.push_back(*it);
  }
  EXPECT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 4u);
  EXPECT_EQ(collected[1], 3u);
  EXPECT_EQ(collected[2], 2u);
}

TEST_F(TensorShapeTest, Equality) {
  // Test operator== with identical shapes
  std::vector<uint64_t> dims1 = {2, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  std::vector<uint64_t> dims2 = {2, 3, 4};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);

  EXPECT_TRUE(shape1 == shape2);
  EXPECT_FALSE(shape1 != shape2);

  // Test operator== with different ranks but equivalent shapes
  std::vector<uint64_t> dims3 = {2, 3, 4, 1, 1};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);
  EXPECT_TRUE(shape1 == shape3);

  // Test operator!= with different shapes
  std::vector<uint64_t> dims4 = {2, 3, 5};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);
  EXPECT_TRUE(shape1 != shape4);
  EXPECT_FALSE(shape1 == shape4);

  // Test isScalar() for various scalar shapes
  TensorShape scalar1;
  EXPECT_TRUE(scalar1.isScalar());

  std::vector<uint64_t> dims5 = {1};
  ArrayRef<uint64_t> dims5Ref(dims5);
  TensorShape scalar2(dims5Ref);
  EXPECT_TRUE(scalar2.isScalar());

  std::vector<uint64_t> dims6 = {1, 1, 1};
  ArrayRef<uint64_t> dims6Ref(dims6);
  TensorShape scalar3(dims6Ref);
  EXPECT_TRUE(scalar3.isScalar());

  // Test isVector() for shapes with at least one dimension > 1
  EXPECT_FALSE(scalar1.isVector());
  EXPECT_FALSE(scalar2.isVector());
  EXPECT_TRUE(shape1.isVector());

  std::vector<uint64_t> dims7 = {2, 1, 1};
  ArrayRef<uint64_t> dims7Ref(dims7);
  TensorShape vector1(dims7Ref);
  EXPECT_TRUE(vector1.isVector());
}

TEST_F(TensorShapeTest, Comparisons) {
  // Test lexicographical ordering from higher to lower dimensions
  // Example: Tensor[2][32] > Tensor[1][32] > Tensor[4000][31]

  std::vector<uint64_t> dims1 = {2, 32};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  std::vector<uint64_t> dims2 = {1, 32};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);

  std::vector<uint64_t> dims3 = {4000, 31};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);

  // Test operator>
  EXPECT_TRUE(shape1 > shape2);
  EXPECT_TRUE(shape2 > shape3);
  EXPECT_TRUE(shape1 > shape3);

  // Test operator<
  EXPECT_TRUE(shape2 < shape1);
  EXPECT_TRUE(shape3 < shape2);
  EXPECT_TRUE(shape3 < shape1);

  // Test operator>=
  EXPECT_TRUE(shape1 >= shape2);
  EXPECT_TRUE(shape1 >= shape1);
  EXPECT_FALSE(shape2 >= shape1);

  // Test operator<=
  EXPECT_TRUE(shape2 <= shape1);
  EXPECT_TRUE(shape1 <= shape1);
  EXPECT_FALSE(shape1 <= shape2);

  // Test with equal shapes - verify ==, >=, and <= are all true
  std::vector<uint64_t> dims4 = {2, 32, 1, 1, 1};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);
  EXPECT_TRUE(shape1 == shape4); // Equality is true
  EXPECT_FALSE(shape1 < shape4); // Less than is false
  EXPECT_FALSE(shape1 > shape4); // Greater than is false
  EXPECT_TRUE(shape1 <= shape4); // Less than or equal is true
  EXPECT_TRUE(shape1 >= shape4); // Greater than or equal is true
}

TEST_F(TensorShapeTest, FlatShape) {
  // Test flatShape() returns product of all dimensions
  std::vector<uint64_t> dims1 = {2, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);
  EXPECT_EQ(shape1.flatShape(), 24u);

  // Test with scalar (should return 1)
  TensorShape scalar;
  EXPECT_EQ(scalar.flatShape(), 1u);

  std::vector<uint64_t> dims2 = {1, 1, 1};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape scalar2(dims2Ref);
  EXPECT_EQ(scalar2.flatShape(), 1u);

  // Test with various shapes
  std::vector<uint64_t> dims3 = {5};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);
  EXPECT_EQ(shape3.flatShape(), 5u);

  std::vector<uint64_t> dims4 = {10, 10};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);
  EXPECT_EQ(shape4.flatShape(), 100u);

  std::vector<uint64_t> dims5 = {2, 3, 4, 5};
  ArrayRef<uint64_t> dims5Ref(dims5);
  TensorShape shape5(dims5Ref);
  EXPECT_EQ(shape5.flatShape(), 120u);
}

TEST_F(TensorShapeTest, Print) {
  // Test print() outputs "Tensor" followed by dimensions in brackets
  // Format: "Tensor[dim0][dim1][dim2]..."
  std::vector<uint64_t> dims1 = {2, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  std::string output;
  raw_string_ostream OS(output);
  shape1.print(OS);
  OS.flush();
  EXPECT_EQ(output, "Tensor[2][3][4]");

  // Test with scalar - should print "Scalar"
  TensorShape scalar;
  output.clear();
  raw_string_ostream OS2(output);
  scalar.print(OS2);
  OS2.flush();
  EXPECT_EQ(output, "Scalar");

  // Test with another shape
  std::vector<uint64_t> dims2 = {32, 4};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);
  output.clear();
  raw_string_ostream OS4(output);
  shape2.print(OS4);
  OS4.flush();
  EXPECT_EQ(output, "Tensor[32][4]");

  // Test printAsSignature with integer type
  // Format: "t" + dimensions separated by 'x' + type suffix
  // For i32: "t2x3x4_i32"
  IntegerType *I32Ty = IntegerType::get(C, 32);
  output.clear();
  raw_string_ostream OS3(output);
  shape1.printAsSignature(OS3, I32Ty);
  OS3.flush();
  EXPECT_EQ(output, "t2x3x4i32");

  // Test printAsSignature with scalar (should be "1")
  output.clear();
  raw_string_ostream OS5(output);
  scalar.printAsSignature(OS5, I32Ty);
  OS5.flush();
  EXPECT_EQ(output, "t1i32");

  // Test printAsSignature with bf16
  Type *BF16Ty = Type::getBFloatTy(C);
  output.clear();
  raw_string_ostream OS6(output);
  shape1.printAsSignature(OS6, BF16Ty);
  OS6.flush();
  EXPECT_EQ(output, "t2x3x4bf16");

  // Test printAsSignature with f16
  Type *F16Ty = Type::getHalfTy(C);
  output.clear();
  raw_string_ostream OS7(output);
  shape1.printAsSignature(OS7, F16Ty);
  OS7.flush();
  EXPECT_EQ(output, "t2x3x4f16");

  // Test printAsSignature with f32
  Type *F32Ty = Type::getFloatTy(C);
  output.clear();
  raw_string_ostream OS8(output);
  shape1.printAsSignature(OS8, F32Ty);
  OS8.flush();
  EXPECT_EQ(output, "t2x3x4f32");

  // Test printAsSignature with f64
  Type *F64Ty = Type::getDoubleTy(C);
  output.clear();
  raw_string_ostream OS9(output);
  shape1.printAsSignature(OS9, F64Ty);
  OS9.flush();
  EXPECT_EQ(output, "t2x3x4f64");

  // Test printAsSignature with different integer widths
  IntegerType *I8Ty = IntegerType::get(C, 8);
  output.clear();
  raw_string_ostream OS10(output);
  shape1.printAsSignature(OS10, I8Ty);
  OS10.flush();
  EXPECT_EQ(output, "t2x3x4i8");

  IntegerType *I60Ty = IntegerType::get(C, 60);
  output.clear();
  raw_string_ostream OS11(output);
  shape1.printAsSignature(OS11, I60Ty);
  OS11.flush();
  EXPECT_EQ(output, "t2x3x4i60");

  // Test printAsSignature with pointer type (default address space 0)
  Type *PtrTy = PointerType::get(C, 0);
  output.clear();
  raw_string_ostream OS12(output);
  shape1.printAsSignature(OS12, PtrTy);
  OS12.flush();
  EXPECT_EQ(output, "t2x3x4ptr");

  // Test printAsSignature with pointer type (non-default address space)
  Type *PtrTy1 = PointerType::get(C, 1);
  output.clear();
  raw_string_ostream OS13(output);
  shape1.printAsSignature(OS13, PtrTy1);
  OS13.flush();
  EXPECT_EQ(output, "t2x3x4ptr1");
}

TEST_F(TensorShapeTest, Broadcast) {
  // Test combineShapeBcast() with compatible shapes
  std::vector<uint64_t> dims1 = {1, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  std::vector<uint64_t> dims2 = {2, 1, 4};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);

  Error err1 = shape1.combineShapeBcast(shape2);
  EXPECT_FALSE(static_cast<bool>(err1));
  // After combining, shape1 should be [2, 3, 4]
  EXPECT_EQ(shape1[0], 2u);
  EXPECT_EQ(shape1[1], 3u);
  EXPECT_EQ(shape1[2], 4u);

  // Test canCombineWith() with compatible shapes
  std::vector<uint64_t> dims3 = {1, 5};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);

  std::vector<uint64_t> dims4 = {3, 1};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);

  Error err2 = shape3.canCombineWith(shape4);
  EXPECT_FALSE(static_cast<bool>(err2));

  // Test canCombineWith() with incompatible shapes
  std::vector<uint64_t> dims5 = {2, 3};
  ArrayRef<uint64_t> dims5Ref(dims5);
  TensorShape shape5(dims5Ref);

  std::vector<uint64_t> dims6 = {4, 5};
  ArrayRef<uint64_t> dims6Ref(dims6);
  TensorShape shape6(dims6Ref);

  Error err3 = shape5.canCombineWith(shape6);
  EXPECT_TRUE(static_cast<bool>(err3));
  consumeError(std::move(err3));

  // Test isBroadcastError() detects incompatible broadcasts
  Error err4 = shape5.isBroadcastError(shape6);
  EXPECT_TRUE(static_cast<bool>(err4));
  consumeError(std::move(err4));

  // Test requiredSplat() identifies dimensions needing broadcast
  std::vector<uint64_t> dims7 = {1, 3, 1};
  ArrayRef<uint64_t> dims7Ref(dims7);
  TensorShape shape7(dims7Ref);

  std::vector<uint64_t> dims8 = {2, 3, 4};
  ArrayRef<uint64_t> dims8Ref(dims8);
  TensorShape shape8(dims8Ref);

  BitVector splat = shape7.requiredSplat(shape8);
  EXPECT_TRUE(splat[0]);  // Dimension 0 needs splat (1 -> 2)
  EXPECT_FALSE(splat[1]); // Dimension 1 matches (3 == 3)
  EXPECT_TRUE(splat[2]);  // Dimension 2 needs splat (1 -> 4)

  // Test broadcastShapeFromAll() with multiple shapes
  std::vector<uint64_t> dims9 = {1, 3, 4};
  ArrayRef<uint64_t> dims9Ref(dims9);
  TensorShape shape9(dims9Ref);

  std::vector<uint64_t> dims10 = {2, 1, 4};
  ArrayRef<uint64_t> dims10Ref(dims10);
  TensorShape shape10(dims10Ref);

  std::vector<uint64_t> dims11 = {2, 3, 1};
  ArrayRef<uint64_t> dims11Ref(dims11);
  TensorShape shape11(dims11Ref);

  std::vector<const TensorShape *> shapes = {&shape9, &shape10, &shape11};
  ArrayRef<const TensorShape *> shapesRef(shapes);

  Expected<TensorShape> result = TensorShape::broadcastShapeFromAll(shapesRef);
  ASSERT_TRUE(static_cast<bool>(result));
  EXPECT_EQ((*result)[0], 2u);
  EXPECT_EQ((*result)[1], 3u);
  EXPECT_EQ((*result)[2], 4u);

  // Test broadcastShapeFromAll() with incompatible shapes
  std::vector<uint64_t> dims12 = {2, 3};
  ArrayRef<uint64_t> dims12Ref(dims12);
  TensorShape shape12(dims12Ref);

  std::vector<uint64_t> dims13 = {4, 5};
  ArrayRef<uint64_t> dims13Ref(dims13);
  TensorShape shape13(dims13Ref);

  std::vector<const TensorShape *> incompatShapes = {&shape12, &shape13};
  ArrayRef<const TensorShape *> incompatShapesRef(incompatShapes);

  Expected<TensorShape> result2 =
      TensorShape::broadcastShapeFromAll(incompatShapesRef);
  EXPECT_FALSE(static_cast<bool>(result2));
  consumeError(result2.takeError());
}

TEST_F(TensorShapeTest, Reductions) {
  // Test reduceDimensions with BitVector specifying dimensions to reduce
  std::vector<uint64_t> dims1 = {2, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  BitVector bv1(3);
  bv1.set(1); // Reduce dimension 1
  bool changed1 = shape1.reduceDimensions(bv1);
  EXPECT_TRUE(changed1);
  EXPECT_EQ(shape1[0], 2u);
  EXPECT_EQ(shape1[1], 1u); // Reduced to 1
  EXPECT_EQ(shape1[2], 4u);

  // Test keepDimensions with BitVector specifying dimensions to keep
  std::vector<uint64_t> dims2 = {2, 3, 4, 5};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);

  BitVector bv2(4);
  bv2.set(0);
  bv2.set(2); // Keep dimensions 0 and 2
  bool changed2 = shape2.keepDimensions(bv2);
  EXPECT_TRUE(changed2);
  EXPECT_EQ(shape2.rank(), 4u);
  EXPECT_EQ(shape2[0], 2u);
  EXPECT_EQ(shape2[1], 1u);
  EXPECT_EQ(shape2[2], 4u);
  EXPECT_EQ(shape2[3], 1u);

  // Test reducedToScalarBy checks if reduction results in scalar
  std::vector<uint64_t> dims3 = {2, 3};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);

  BitVector bv3(2);
  bv3.set(0);
  bv3.set(1); // Reduce all dimensions
  EXPECT_TRUE(shape3.reducedToScalarBy(bv3));

  BitVector bv4(2);
  bv4.set(0); // Reduce only dimension 0
  EXPECT_FALSE(shape3.reducedToScalarBy(bv4));

  // Test nonEmptyDims()
  std::vector<uint64_t> dims4 = {2, 1, 3, 1};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);

  BitVector nonEmpty = shape4.nonEmptyDims();
  EXPECT_TRUE(nonEmpty[0]);  // 2 > 1
  EXPECT_FALSE(nonEmpty[1]); // 1 == 1
  EXPECT_TRUE(nonEmpty[2]);  // 3 > 1
  EXPECT_FALSE(nonEmpty[3]); // 1 == 1

  // Test bothNonEmptyDims()
  std::vector<uint64_t> dims5 = {2, 1, 3};
  ArrayRef<uint64_t> dims5Ref(dims5);
  TensorShape shape5(dims5Ref);

  std::vector<uint64_t> dims6 = {2, 4, 1};
  ArrayRef<uint64_t> dims6Ref(dims6);
  TensorShape shape6(dims6Ref);

  BitVector bothNonEmpty = shape5.bothNonEmptyDims(shape6);
  EXPECT_TRUE(bothNonEmpty[0]);  // Both have 2
  EXPECT_FALSE(bothNonEmpty[1]); // shape5 has 1
  EXPECT_FALSE(bothNonEmpty[2]); // shape6 has 1

  // Test reductionDimensionsBeforeBroadcast()
  std::vector<uint64_t> dims7 = {2, 3, 4};
  ArrayRef<uint64_t> dims7Ref(dims7);
  TensorShape shape7(dims7Ref);

  std::vector<uint64_t> dims8 = {1, 3, 4};
  ArrayRef<uint64_t> dims8Ref(dims8);
  TensorShape shape8(dims8Ref);

  BitVector reductions = shape7.reductionDimensionsBeforeBroadcast(shape8);
  EXPECT_TRUE(reductions[0]);  // Need to reduce dimension 0 (2 -> 1)
  EXPECT_FALSE(reductions[1]); // Dimension 1 matches
  EXPECT_FALSE(reductions[2]); // Dimension 2 matches

  // Test testBothDims()
  auto testFunc = [](uint64_t a, uint64_t b) { return a == b; };
  BitVector testResult = shape5.testBothDims(shape6, testFunc);
  EXPECT_TRUE(testResult[0]);  // 2 == 2
  EXPECT_FALSE(testResult[1]); // 1 != 4
  EXPECT_FALSE(testResult[2]); // 3 != 1
}

TEST_F(TensorShapeAttributeTest, FunctionShapeAttributes) {
  IRBuilder<> IRB(C);
  Module M("TestModule", C);
  Type *I64Ty = IRB.getInt64Ty();
  FunctionType *FTy = FunctionType::get(I64Ty, {I64Ty, I64Ty}, false);
  Function *F = Function::Create(
      FTy, GlobalValue::LinkageTypes::ExternalLinkage, "TestF", M);
  BasicBlock *BB = BasicBlock::Create(C, "entry", F);
  IRB.SetInsertPoint(BB);

  // Give Ripple a five-dimensional tensor space.
  Function *SetShapeFn = Intrinsic::getOrInsertDeclaration(
      &M, Intrinsic::ripple_block_setshape, {I64Ty});
  SmallVector<Value *, Ripple::RippleIntrinsicsMaxDims + 1> SetShapeArgs;
  SetShapeArgs.push_back(IRB.getInt64(0));
  for (unsigned I = 0; I != Ripple::RippleIntrinsicsMaxDims; ++I)
    SetShapeArgs.push_back(IRB.getInt64(I < 5 ? 2 : 1));
  IRB.CreateCall(SetShapeFn, SetShapeArgs);
  IRB.CreateRet(IRB.getInt64(0));

  SpecializationsPending.insert(F);
  auto RippleExpander = createRipple(*F);
  ASSERT_TRUE(RippleExpander);
  EXPECT_FALSE(Ripple::hasRippleShapeAttributes(*F));

  TensorShape ArgShape1({2, 3, 4, 1, 1});
  TensorShape ArgShape2(5);
  F->addFnAttr("ripple.function.shape", "2,3,4;;2,3,4,5,6");
  EXPECT_TRUE(Ripple::hasRippleShapeAttributes(*F));
  EXPECT_EQ(F->getFnAttribute("ripple.function.shape").getValueAsString(),
            "2,3,4;;2,3,4,5,6");

  bool WaitingForSpecialization = false;
  ASSERT_FALSE(RippleExpander->propagateShapes(WaitingForSpecialization));
  EXPECT_FALSE(WaitingForSpecialization);
  EXPECT_EQ(RippleExpander->getRippleShape(static_cast<Value *>(F->getArg(0))),
            ArgShape1);
  EXPECT_EQ(RippleExpander->getRippleShape(static_cast<Value *>(F->getArg(1))),
            ArgShape2);

  RippleExpander.reset();
  SpecializationsPending.clear();
}

TEST_F(TensorShapeAttributeTest, BlockShapeSpecializationAttributes) {
  IRBuilder<> IRB(C);
  Module M("TestModule", C);
  Type *I64Ty = IRB.getInt64Ty();
  Type *PtrTy = IRB.getPtrTy();

  FunctionType *CalleeTy = FunctionType::get(I64Ty, {PtrTy, I64Ty}, false);
  Function *Callee =
      Function::Create(CalleeTy, GlobalValue::InternalLinkage, "callee", M);
  BasicBlock *CalleeBB = BasicBlock::Create(C, "entry", Callee);
  IRB.SetInsertPoint(CalleeBB);
  IRB.CreateRet(Callee->getArg(1));

  auto CreateCaller = [&](StringRef Name, Type *IntTy, uint64_t Dim0,
                          uint64_t Dim1) {
    FunctionType *CallerTy = FunctionType::get(I64Ty, false);
    Function *Caller =
        Function::Create(CallerTy, GlobalValue::ExternalLinkage, Name, M);
    BasicBlock *CallerBB = BasicBlock::Create(C, "entry", Caller);
    IRB.SetInsertPoint(CallerBB);
    Function *SetShapeFn = Intrinsic::getOrInsertDeclaration(
        &M, Intrinsic::ripple_block_setshape, {IntTy});
    SmallVector<Value *, Ripple::RippleIntrinsicsMaxDims + 1> Args;
    Args.push_back(ConstantInt::get(IntTy, 0));
    for (unsigned I = 0; I != Ripple::RippleIntrinsicsMaxDims; ++I) {
      uint64_t Dim = I == 0 ? Dim0 : I == 1 ? Dim1 : 1;
      Args.push_back(ConstantInt::get(IntTy, Dim));
    }
    Value *BlockShape = IRB.CreateCall(SetShapeFn, Args);
    Value *Result = IRB.CreateCall(Callee, {BlockShape, IRB.getInt64(1)});
    IRB.CreateRet(Result);
    return Caller;
  };

  Function *Block2x3I64 = CreateCaller("block_2x3_i64", I64Ty, 2, 3);
  Function *Block2x3I32 = CreateCaller("block_2x3_i32", IRB.getInt32Ty(), 2, 3);
  Function *Block4x3I64 = CreateCaller("block_4x3_i64", I64Ty, 4, 3);
  Function *BlockScalarI64 = CreateCaller("block_scalar_i64", I64Ty, 1, 1);

  scope_exit ClearSpecializations([&] { SpecializationsPending.clear(); });
  auto Propagate = [&](Function *Caller) {
    auto RippleExpander = createRipple(*Caller);
    if (!RippleExpander)
      return false;
    bool WaitingForSpecialization = false;
    if (Error E = RippleExpander->propagateShapes(WaitingForSpecialization)) {
      ADD_FAILURE() << toString(std::move(E));
      return false;
    }
    return WaitingForSpecialization;
  };

  ASSERT_TRUE(Propagate(Block2x3I64));
  ASSERT_EQ(SpecializationsPending.size(), 1u);
  Function *Specialization = *SpecializationsPending.begin();
  EXPECT_TRUE(Ripple::hasRippleShapeAttributes(*Specialization));
  EXPECT_EQ(Specialization->getFnAttribute("ripple.function.shape")
                .getValueAsString(),
            ";;;;");
  EXPECT_EQ(Specialization->getFnAttribute("ripple.function.block-shape")
                .getValueAsString(),
            "0,0,2,3");

  // Equivalent descriptors with different integer widths share the pending
  // specialization, while different extents request a new one.
  ASSERT_TRUE(Propagate(Block2x3I32));
  EXPECT_EQ(SpecializationsPending.size(), 1u);
  ASSERT_TRUE(Propagate(Block4x3I64));
  ASSERT_EQ(SpecializationsPending.size(), 2u);
  ASSERT_TRUE(Propagate(BlockScalarI64));
  ASSERT_EQ(SpecializationsPending.size(), 3u);

  SmallVector<std::string, 3> BlockShapeAttributes;
  for (Function *Pending : SpecializationsPending)
    BlockShapeAttributes.push_back(
        Pending->getFnAttribute("ripple.function.block-shape")
            .getValueAsString()
            .str());
  llvm::sort(BlockShapeAttributes);
  EXPECT_EQ(BlockShapeAttributes[0], "0,0");
  EXPECT_EQ(BlockShapeAttributes[1], "0,0,2,3");
  EXPECT_EQ(BlockShapeAttributes[2], "0,0,4,3");
}

TEST_F(TensorShapeAttributeTest, NonConstantBlockShapeRejected) {
  IRBuilder<> IRB(C);
  Module M("TestModule", C);
  Type *I64Ty = IRB.getInt64Ty();
  FunctionType *FTy = FunctionType::get(IRB.getVoidTy(), {I64Ty}, false);
  Function *F = Function::Create(FTy, GlobalValue::ExternalLinkage,
                                 "nonconstant_block_shape", M);
  BasicBlock *BB = BasicBlock::Create(C, "entry", F);
  IRB.SetInsertPoint(BB);

  Function *SetShapeFn = Intrinsic::getOrInsertDeclaration(
      &M, Intrinsic::ripple_block_setshape, {I64Ty});
  SmallVector<Value *, Ripple::RippleIntrinsicsMaxDims + 1> Args;
  Args.push_back(IRB.getInt64(0));
  Args.push_back(F->getArg(0));
  Args.push_back(IRB.getInt64(2));
  while (Args.size() != Ripple::RippleIntrinsicsMaxDims + 1)
    Args.push_back(IRB.getInt64(1));
  IRB.CreateCall(SetShapeFn, Args);
  IRB.CreateRetVoid();

  auto RippleExpander = createRipple(*F);
  ASSERT_TRUE(RippleExpander);
  Error E = RippleExpander->validate();
  ASSERT_TRUE(static_cast<bool>(E));
  EXPECT_NE(toString(std::move(E))
                .find("block shape operand 1 must be a constant integer"),
            std::string::npos);
}

TEST_F(TensorShapeTest, Indexing) {
  // Test getOffsetAt() - returns offset in number of elements from start
  // For shape [2, 3, 4], the layout is row-major from innermost dimension
  std::vector<uint64_t> dims1 = {2, 3, 4};
  ArrayRef<uint64_t> dims1Ref(dims1);
  TensorShape shape1(dims1Ref);

  // Test corner cases
  std::vector<size_t> coord1 = {0, 0, 0};
  EXPECT_EQ(shape1.getOffsetAt(coord1), 0u);

  // Test various coordinates
  std::vector<size_t> coord2 = {1, 0, 0};
  EXPECT_EQ(shape1.getOffsetAt(coord2), 1u);

  std::vector<size_t> coord3 = {0, 1, 0};
  EXPECT_EQ(shape1.getOffsetAt(coord3), 2u);

  std::vector<size_t> coord4 = {1, 1, 0};
  EXPECT_EQ(shape1.getOffsetAt(coord4), 3u);

  std::vector<size_t> coord5 = {0, 0, 1};
  EXPECT_EQ(shape1.getOffsetAt(coord5), 6u);

  std::vector<size_t> coord6 = {1, 2, 3};
  EXPECT_EQ(shape1.getOffsetAt(coord6), 23u); // Last element

  // Test with 2D shape
  std::vector<uint64_t> dims2 = {3, 4};
  ArrayRef<uint64_t> dims2Ref(dims2);
  TensorShape shape2(dims2Ref);

  std::vector<size_t> coord7 = {0, 0};
  EXPECT_EQ(shape2.getOffsetAt(coord7), 0u);

  std::vector<size_t> coord8 = {2, 3};
  EXPECT_EQ(shape2.getOffsetAt(coord8), 11u); // Last element of [3, 4]

  // Test foreachIndex() - calls function for each valid coordinate
  std::vector<uint64_t> dims3 = {2, 3};
  ArrayRef<uint64_t> dims3Ref(dims3);
  TensorShape shape3(dims3Ref);

  std::vector<std::vector<size_t>> collectedCoords;
  shape3.foreachIndex([&collectedCoords](ArrayRef<size_t> coord) {
    collectedCoords.push_back(std::vector<size_t>(coord.begin(), coord.end()));
  });

  // Should have 2 * 3 = 6 coordinates
  EXPECT_EQ(collectedCoords.size(), 6u);

  // Verify all coordinates are present
  EXPECT_EQ(collectedCoords[0], std::vector<size_t>({0, 0}));
  EXPECT_EQ(collectedCoords[1], std::vector<size_t>({1, 0}));
  EXPECT_EQ(collectedCoords[2], std::vector<size_t>({0, 1}));
  EXPECT_EQ(collectedCoords[3], std::vector<size_t>({1, 1}));
  EXPECT_EQ(collectedCoords[4], std::vector<size_t>({0, 2}));
  EXPECT_EQ(collectedCoords[5], std::vector<size_t>({1, 2}));

  // Test foreachIndex with scalar
  TensorShape scalar;
  size_t scalarCount = 0;
  scalar.foreachIndex([&scalarCount](ArrayRef<size_t> coord) {
    scalarCount++;
    EXPECT_EQ(coord.size(), 0u);
  });
  EXPECT_EQ(scalarCount, 0u); // Scalar has no "coordinate" (empty)

  // Test foreachIndex with 1D shape
  std::vector<uint64_t> dims4 = {3};
  ArrayRef<uint64_t> dims4Ref(dims4);
  TensorShape shape4(dims4Ref);

  std::vector<std::vector<size_t>> coords1D;
  shape4.foreachIndex([&coords1D](ArrayRef<size_t> coord) {
    coords1D.push_back(std::vector<size_t>(coord.begin(), coord.end()));
  });

  EXPECT_EQ(coords1D.size(), 3u);
  EXPECT_EQ(coords1D[0], std::vector<size_t>({0}));
  EXPECT_EQ(coords1D[1], std::vector<size_t>({1}));
  EXPECT_EQ(coords1D[2], std::vector<size_t>({2}));
}

} // namespace
} // namespace llvm
