#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include <cxxabi.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace std;

namespace llfi
{

  // Command Line option for Fake Quant
  cl::opt<std::string>
      fakeQuant("fakeQuant", cl::desc("Specify the mode for fake quantization."),
                cl::init("None"));
  cl::opt<int>
      minPercentileThreshold("minPercentileThreshold", cl::desc("Specify the minimum threshold to remove outliers within percentile metrics"),
                             cl::init(0));

  cl::opt<int>
      maxPercentileThreshold("maxPercentileThreshold", cl::desc("Specify the maximum threshold to remove outliers within percentile metrics"),
                             cl::init(100));

  cl::opt<int>
      bitWidth("bitWidth", cl::desc("Specify the bit width for Quantization"),
               cl::init(8));

  unordered_set<Instruction *> deleteInst;

  std::map<std::string, int64_t> ONNXOperatorId = {
      {"conv", 1986948931},
      {"relu", 1970038098},
      {"maxpool", 30521821366870349},
      {"matmul", 119251066446157},
      {"add", 6579265},
      {"avgpool", 30521821365761601},
      {"softmax", 33884119937478483},
      {"loop", 1886351180},
      {"nonmaxs", 23494782373228366},
      {"unsqueeze", 28540527736745557}};

  int totalNumberOfLayers = 0;
  int currentNumberOfLayers = 0;

  /*

    The Plan for this Module Pass

    After converting the onnx to LLVM IR -> Target Conv Layer -> Specifically y =
    Wx + B
    -> Qunatizing W and X to int and after the conversion Dequantise it to float

   */

  void insertNewFunction(llvm::Module &M)
  {
    llvm::Function *OrigFunction = M.getFunction("injectFault0");
    if (!OrigFunction)
    {
      errs() << "Function injectFault0 not found!\n";
      return;
    }

    // Creating a clone of the function
    llvm::ValueToValueMapTy VMap;
    llvm::Function *ClonedFunction = llvm::CloneFunction(OrigFunction, VMap);
    if (!ClonedFunction)
    {
      errs() << "Could not clone the function!\n";
      return;
    }

    // Rename the cloned function
    ClonedFunction->setName("FakeQuantizationInjectionFunction");

    // Ensure the new function is added to the module
    // M.getFunctionList().push_back(ClonedFunction);

    for (BasicBlock &BB : *ClonedFunction)
    {
      for (Instruction &I : BB)
      {

        if (AllocaInst *AI = dyn_cast<AllocaInst>(&I))
        {

          LLVMContext &Context = AI->getContext();
          IRBuilder<> Builder(AI);

          // Create a new alloca instruction for int type
          AllocaInst *NewAI = Builder.CreateAlloca(Type::getInt32Ty(Context),
                                                   nullptr, AI->getName());

          // Copy alignment from the old alloca to the new one
          NewAI->setAlignment(AI->getAlign());

          // Replace all uses of the old alloca with the new one
          AI->replaceAllUsesWith(NewAI);

          // Remove the old alloca from the basic block
          // AI->eraseFromParent();
          deleteInst.insert(AI);
        }
        else if (StoreInst *SI = dyn_cast<StoreInst>(&I))
        {
          IRBuilder<> Builder(SI);
          Value *StoredVal = SI->getValueOperand();
          Value *CastToInt = Builder.CreateFPToSI(
              StoredVal, Type::getInt32Ty(ClonedFunction->getContext()), "cast_to_int");
          StoreInst *NewStore =
              Builder.CreateStore(CastToInt, SI->getPointerOperand());
          NewStore->setAlignment(Align(4));
          deleteInst.insert(SI);
        }
        else if (auto *LI = dyn_cast<LoadInst>(&I))
        {

          IRBuilder<> Builder(LI);
          // Create a new load instruction to load an integer
          LoadInst *NewLoad = Builder.CreateLoad(
              Type::getInt32Ty(ClonedFunction->getContext()), LI->getPointerOperand(),
              LI->getName() + "_int");
          NewLoad->setAlignment(Align(4));

          // Convert loaded integer to float
          Value *ConvertedFloat = Builder.CreateSIToFP(
              NewLoad, Type::getFloatTy(ClonedFunction->getContext()), "converted_to_float");

          // Replace all uses of the original load with the new float value
          LI->replaceAllUsesWith(ConvertedFloat);

          // Remove the old load from its parent block
          deleteInst.insert(LI);
        }
      }
    }

    bool foundFakeQuantInst = false;
    for (Function &F : M)
    {
      for (BasicBlock &BB : F)
      {
        for (Instruction &I : BB)
        {
          if (CallInst *callInst = dyn_cast<CallInst>(&I))
          {
            if (Function *calledFunction = callInst->getCalledFunction())
            {
              if (calledFunction->getName() == "Quantize")
              {
                foundFakeQuantInst = true;
                continue;
              }
              else if (calledFunction->getName() ==
                       "FakeQuantIntegerBasedAddition")
              {
                foundFakeQuantInst = true;
                continue;
              }

              else if (calledFunction->getName() == "injectFault0" &&
                       foundFakeQuantInst)
              {
                IRBuilder<> Builder(callInst);
                std::vector<Value *> args(callInst->arg_begin(),
                                          callInst->arg_end());

                Function *newFunc = callInst->getModule()->getFunction(
                    "FakeQuantizationInjectionFunction");

                CallInst *newCall = Builder.CreateCall(newFunc, args);

                callInst->replaceAllUsesWith(newCall);

                deleteInst.insert(callInst);
                foundFakeQuantInst = false;
              }
            }
          }
        }
      }
    }
  }

  string getOperandName(Instruction *I)
  {
    string operandName;

    // Check if the operand has a name before printing
    if (I->hasName())
    {
      operandName = '%' + I->getName().str();
    }
    else
    {
      raw_string_ostream resultStream(operandName);
      I->printAsOperand(resultStream, false);
      resultStream.flush();
    }

    return operandName;
  }

  void findTotalLayers(Module &M)
  {
    int numberOfLayers = 0;
    for (Function &F : M)
    {
      for (auto &I : instructions(F))
      {

        if (CallInst *callInst = dyn_cast<CallInst>(&I))
        {
          if (callInst->getCalledFunction() &&
              callInst->getCalledFunction()->getName() == "OMInstrumentPoint")
          {

            int64_t opCode =
                dyn_cast<ConstantInt>(callInst->getArgOperand(0))->getSExtValue();

            if (ONNXOperatorId[fakeQuant] == opCode)
            {
              numberOfLayers++;
            }
          }
        }
      }
    }
    totalNumberOfLayers = numberOfLayers / 2;
  }

  void insertFakeQuantForInjectFault(Function &F)
  {

    LLVMContext &Context = F.getContext();
    Module *M = F.getParent();

    string qunatize = (fakeQuant == "conv") ? "Quantize" : "QuantizeMatMul";

    FunctionCallee Quantize = M->getOrInsertFunction(
        qunatize, Type::getFloatTy(Context), Type::getFloatTy(Context),
        Type::getFloatTy(Context), Type::getInt32Ty(Context),
        Type::getInt32Ty(Context));

    FunctionCallee FakeQuantIntegerBasedAdditionFunction = M->getOrInsertFunction(
        "FakeQuantIntegerBasedAddition", Type::getFloatTy(Context), Type::getFloatTy(Context),
        Type::getFloatTy(Context));

    FunctionCallee FakeQunatDequantizeAndBiasAdditionFunction = M->getOrInsertFunction(
        "FakeQunatDequantizeAndBiasAddition", Type::getFloatTy(Context), Type::getFloatTy(Context),
        Type::getFloatTy(Context));

    bool foundLayerDef = false;

    int numberofFadd = 0;

    for (auto &I : instructions(F))
    {

      if (CallInst *callInst = dyn_cast<CallInst>(&I))
      {
        if (callInst->getCalledFunction() &&
            callInst->getCalledFunction()->getName() == "OMInstrumentPoint")
        {

          int64_t opCode =
              dyn_cast<ConstantInt>(callInst->getArgOperand(0))->getSExtValue();

          if (ONNXOperatorId[fakeQuant] == opCode)
          {

            int64_t status =
                dyn_cast<ConstantInt>(callInst->getArgOperand(1))->getSExtValue();
            if (status == 1)
            {
              foundLayerDef = true;
              currentNumberOfLayers++;
              continue;
            }
            else
            {
              foundLayerDef = false;
              numberofFadd = 0;
            }
          }
        }
      }
      if (foundLayerDef)
      {

        if (auto *op = dyn_cast<Instruction>(&I))
        {

          if (op->getOpcode() == Instruction::FMul)
          {

            IRBuilder<> Builder(op);

            Value *W = op->getOperand(0);
            Value *X = op->getOperand(1);
            Value *currentLayerIndex = ConstantInt::get(Type::getInt32Ty(Context),
                                                        currentNumberOfLayers);
            Value *_totalNumberOfLayers =
                ConstantInt::get(Type::getInt32Ty(Context), totalNumberOfLayers);

            Value *newInst = Builder.CreateCall(
                Quantize, {W, X, currentLayerIndex, _totalNumberOfLayers});

            string operandName = getOperandName(op);


            op->setName(operandName + "_old");


            // Replacing the old float to new float obtained from Fake
            // Quantization
            op->replaceAllUsesWith(newInst);
            deleteInst.insert(op);
          }
          else if (op->getOpcode() == Instruction::FAdd && fakeQuant == "conv")
          {
            numberofFadd++;
            if (numberofFadd == 2)
            {

              IRBuilder<> Builder(op);
              Value *op1 = op->getOperand(0);
              Value *op2 = op->getOperand(1);

              Value *newInst = Builder.CreateCall(FakeQuantDequnatizeAndBiasAdditionFunction, {op1, op2});
              op->replaceAllUsesWith(newInst);
              deleteInst.insert(op);
            }
            else
            {
              IRBuilder<> Builder(op);
              Value *op1 = op->getOperand(0);
              Value *op2 = op->getOperand(1);

              Value *newInst = Builder.CreateCall(FakeQuantIntegerBasedAdditionFunction, {op1, op2});

              op->replaceAllUsesWith(newInst);
              deleteInst.insert(op);
            }
          }
        }
      }
    }
  }

  void insertFakeQuantForProfiling(Function &F)
  {

    LLVMContext &Context = F.getContext();
    Module *M = F.getParent();

    FunctionCallee GetWAndXFunc = M->getOrInsertFunction(
        "getWAndX", Type::getFloatTy(Context), Type::getFloatTy(Context),
        Type::getFloatTy(Context), Type::getInt32Ty(Context),
        Type::getInt32Ty(Context));

    FunctionCallee finished = M->getOrInsertFunction(
        "finished", Type::getVoidTy(Context), Type::getInt32Ty(Context),
        Type::getInt32Ty(Context), Type::getInt32Ty(Context), Type::getInt32Ty(Context), Type::getInt32Ty(Context));


    bool foundLayerDef = false;
    int numberofFadd = 0;

    for (auto &I : instructions(F))
    {

      if (CallInst *callInst = dyn_cast<CallInst>(&I))
      {
        if (callInst->getCalledFunction() &&
            callInst->getCalledFunction()->getName() == "OMInstrumentPoint")
        {

          int64_t opCode =
              dyn_cast<ConstantInt>(callInst->getArgOperand(0))->getSExtValue();

          if (ONNXOperatorId[fakeQuant] == opCode)
          {

            int64_t status =
                dyn_cast<ConstantInt>(callInst->getArgOperand(1))->getSExtValue();
            if (status == 1)
            {
              foundLayerDef = true;
              currentNumberOfLayers++;
              continue;
            }
            else
            {
              foundLayerDef = false;

              IRBuilder<> Builder(callInst);
              Value *currentLayerIndex = ConstantInt::get(
                  Type::getInt32Ty(Context), currentNumberOfLayers);
              Value *_totalNumberOfLayers = ConstantInt::get(
                  Type::getInt32Ty(Context), totalNumberOfLayers);

              Value *minPercentileValue = ConstantInt::get(
                  Type::getInt32Ty(Context), minPercentileThreshold);
              Value *maxPercentileValue = ConstantInt::get(
                  Type::getInt32Ty(Context), maxPercentileThreshold);
              Value *bitWidthValue = ConstantInt::get(
                  Type::getInt32Ty(Context), bitWidth);

              Instruction *newInst = Builder.CreateCall(finished,
                                                        {currentLayerIndex, _totalNumberOfLayers, minPercentileValue, maxPercentileValue, bitWidthValue});
            }
          }
        }
      }
      if (foundLayerDef)
      {

        if (auto *op = dyn_cast<Instruction>(&I))
        {

          if (op->getOpcode() == Instruction::FMul)
          {

            IRBuilder<> Builder(op);

            Value *W = op->getOperand(0);
            Value *X = op->getOperand(1);
            Value *currentLayerIndex = ConstantInt::get(Type::getInt32Ty(Context),
                                                        currentNumberOfLayers);
            Value *_totalNumberOfLayers =
                ConstantInt::get(Type::getInt32Ty(Context), totalNumberOfLayers);

            Value *newInst = Builder.CreateCall(
                GetWAndXFunc, {W, X, currentLayerIndex, _totalNumberOfLayers});

            string operandName = getOperandName(op);


            op->setName(operandName + "_old");

            op->replaceAllUsesWith(newInst);
            deleteInst.insert(op);
          }
        }
      }
    }
  }

  void insertFakeQuantInst(Module &M, bool IsProfiling)
  {
    findTotalLayers(M);
    currentNumberOfLayers = 0;

    for (Function &F : M)
    {
      if (!F.isDeclaration())
      {
        if (IsProfiling)
        {

          insertFakeQuantForProfiling(F);
        }
        else
        {
          insertFakeQuantForInjectFault(F);
        }
      }
    }

    if (!IsProfiling)
    {
      insertNewFunction(M);
    }

    for (Instruction *inst : deleteInst)
    {
      inst->eraseFromParent();
    }
  }

} // namespace llfi
