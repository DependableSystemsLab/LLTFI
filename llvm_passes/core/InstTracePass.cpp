/***************
InstTracePass.cpp
Author: Sam Coulter
  This llvm pass is part of the greater LLFI framework

  Run the pass with the opt -InstTrace option after loading LLFI.so

  This pass injects a function call before every non-void-returning,
  non-phi-node instruction that prints trace information about the executed
  instruction to a file specified during the pass.
***************/

#include <vector>
#include <cmath>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "Utils.h"
#include "InstTracePass.h"

cl::opt<bool> debugtrace("debugtrace",
              cl::desc("Print tracing instrucmented instruction information"),
              cl::init(false));
cl::opt<int> maxtrace( "maxtrace",
    cl::desc("Maximum number of dynamic instructions that will be traced after fault injection"),
            cl::init(1000));

using namespace llvm;

namespace llfi {

  bool InstTrace::doFinalization(Module &M) {
    //Dont forget to delete the output filename string!
    Function* mainfunc = M.getFunction("main");
    if (mainfunc == NULL) {
      errs() << "ERROR: Function main does not exist, " <<
          "which is required by LLFI\n";
      exit(1);
    }

    LLVMContext &context = M.getContext();
    FunctionType *postinjectfunctype =
        FunctionType::get(Type::getVoidTy(context), false);
    FunctionCallee postracingfunc =
        M.getOrInsertFunction("postTracing", postinjectfunctype);

    std::set<Instruction*> exitinsts;
    getProgramExitInsts(M, exitinsts);
    assert (exitinsts.size() != 0
            && "Program does not have explicit exit point");

    for (std::set<Instruction*>::iterator it = exitinsts.begin();
         it != exitinsts.end(); ++it) {
      Instruction *term = *it;
      CallInst::Create(postracingfunc, "", term);
    }

    return true;
  }

  long InstTrace::fetchLLFIInstructionID(Instruction *targetInst) {
    return llfi::getLLFIIndexofInst(targetInst);
  }

  Instruction* InstTrace::getInsertPoint(Instruction* llfiIndexedInst) {
    Instruction *insertPoint;
    if (!llfiIndexedInst->isTerminator()) {
      insertPoint = llfi::getInsertPtrforRegsofInst(llfiIndexedInst, llfiIndexedInst);
      // if insert point is a call to inject fault, insert printInstTrace after the injectFault call
      // iff injectFault occurs AFTER the targeted instruction (ie. dst targeted)
      insertPoint = changeInsertPtrIfInjectFaultInst(insertPoint);
    } else {
      // if terminator, insert before function
      insertPoint = llfiIndexedInst;
    }
    return insertPoint;
  }

  bool InstTrace::runOnFunction(Function &F) {
    //Create handles to the functions parent module and context
    LLVMContext& context = F.getContext();
    Module *M = F.getParent();

    //iterate through each instruction of the function
    inst_iterator lastInst;
    for (inst_iterator instIterator = inst_begin(F),
         lastInst = inst_end(F);
         instIterator != lastInst; ++instIterator) {

        //Print some Debug Info as the pass is being run
      Instruction *inst = &*instIterator;

      if (debugtrace) {
        if (!llfi::isLLFIIndexedInst(inst)) {
          errs() << "Instruction " << *inst << " was not indexed\n";
        } else {
          errs() << "Instruction " << *inst << " was indexed\n";
        }
      }

      if (llfi::isLLFIIndexedInst(inst)) {

        //Find instrumentation point for current instruction
        Instruction *insertPoint = getInsertPoint(inst);

        //Skip instrumentation for terminating instructions
        if (insertPoint->isTerminator()) {
          continue;
        }

        //======== Find insertion location for alloca QINING @SET 15th============
        Instruction* alloca_insertPoint = inst->getParent()->getParent()->begin()->getFirstNonPHIOrDbgOrLifetime();
        //========================================================================


        //Fetch size of instruction value
        //The size must be rounded up before conversion to bytes because some data in llvm
        //can be like 1 bit if it only needs 1 bit out of an 8bit/1byte data type
        float bitSize;
        AllocaInst* ptrInst;
        if (inst->getType() != Type::getVoidTy(context)) {
          //insert an instruction Allocate stack memory to store/pass instruction value
          ptrInst = new AllocaInst(inst->getType(), 0, "llfi_trace",
                                   alloca_insertPoint);
          //Insert an instruction to Store the instruction Value!
          new StoreInst(inst, ptrInst, insertPoint);

          const DataLayout &td = F.getParent()->getDataLayout();
          bitSize = (float)td.getTypeSizeInBits(inst->getType());
        }
        else {
          ptrInst = new AllocaInst(Type::getInt32Ty(context), 0, "llfi_trace",
                                   alloca_insertPoint);
          new StoreInst(ConstantInt::get(IntegerType::get(context, 32), 0),
                        ptrInst, insertPoint);
          bitSize = 32;
        }
        int byteSize = (int)ceil(bitSize / 8.0);

        //Insert instructions to allocate stack memory for opcode name
        const char* opcodeNamePt = inst->getOpcodeName();
        const std::string str(inst->getOpcodeName());
        ArrayRef<uint8_t> opcode_name_array_ref((uint8_t*)opcodeNamePt, str.size() + 1);
        //llvm::Value* OPCodeName = llvm::ConstantArray::get(context, opcode_name_array_ref);
        llvm::Value* OPCodeName = llvm::ConstantDataArray::get(context, opcode_name_array_ref);
        /********************************/

        AllocaInst *OPCodePtr = new AllocaInst(
            OPCodeName->getType(), 0, "llfi_trace", alloca_insertPoint);
        new StoreInst(OPCodeName, OPCodePtr, insertPoint);

        //Create the decleration of the printInstTracer Function
        std::vector<Type*> parameterVector(5);
        parameterVector[0] = Type::getInt32Ty(context); //ID
        parameterVector[1] = OPCodePtr->getType();
        //======== opcode_str QINING @SET 15th============
        //parameterVector[1] = PointerType::get(Type::getInt8Ty(context), 0);     //Ptr to OpCode
        //================================================
        parameterVector[2] = Type::getInt32Ty(context); //Size of Inst Value
        parameterVector[3] = ptrInst->getType();    //Ptr to Inst Value
        parameterVector[4] = Type::getInt32Ty(context); //Int of max traces

        //LLVM 3.3 Upgrade
        ArrayRef<Type*> parameterVector_array_ref(parameterVector);

        FunctionType* traceFuncType = FunctionType::get(Type::getVoidTy(context), 
                                                        parameterVector_array_ref, false);
        FunctionCallee traceFunc =
            M->getOrInsertFunction("printInstTracer", traceFuncType);

        //Insert the tracing function, passing it the proper arguments
        std::vector<Value*> traceArgs;
        //Fetch the LLFI Instruction ID:
        ConstantInt* IDConstInt = ConstantInt::get(IntegerType::get(context, 32), 
                                                   fetchLLFIInstructionID(inst));

        ConstantInt* instValSize = ConstantInt::get(
                                      IntegerType::get(context, 32), byteSize);

        //Fetch maxtrace number:
        ConstantInt* maxTraceConstInt =
        ConstantInt::get(IntegerType::get(context, 32), maxtrace);

        //======== opcode_str QINING @SET 15th============
        //string opcode_str = fi_inst->getOpcodeName();
        //GlobalVariable* opcode_str_gv = findOrCreateGlobalNameString(M, opcode_str);
        //vector<Constant*> indices_for_gep(2);
        //indices_for_gep[0] = ConstantInt::get(Type::getInt32Ty(context),0);
        //indices_for_gep[1] = ConstantInt::get(Type::getInt32Ty(context),0);
        //ArrayRef<Constant*> gep_expr_ref(indices_for_gep);
        //Constant* gep_expr_opcode = ConstantExpr::getGetElementPtr(opcode_str_gv, gep_expr_ref);
        //================================================

        //Load All Arguments
        traceArgs.push_back(IDConstInt);
        traceArgs.push_back(OPCodePtr);
        traceArgs.push_back(instValSize);
        traceArgs.push_back(ptrInst);
        traceArgs.push_back(maxTraceConstInt);

        //LLVM 3.3 Upgrade
        ArrayRef<Value*> traceArgs_array_ref(traceArgs);

        //Create the Function
        CallInst::Create(traceFunc, traceArgs_array_ref, "", insertPoint);
      }
    }//Function Iteration

    return true; //Tell LLVM that the Function was modified
  }//RunOnFunction

//Register the pass with the llvm
char InstTrace::ID = 0;
static RegisterPass<InstTrace> X("insttracepass",
    "Add tracing function calls in program to trace instruction value at runtime",
    false, false);

}//namespace llfi

