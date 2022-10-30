//This pass is run after the transform pass for inserting hooks
//for fault injection
#ifndef PROFILING_PASS_H
#define PROFILING_PASS_H

#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <iostream>

using namespace llvm;

namespace llfi {

  // For legacy PM
  class LegacyProfilingPass: public ModulePass {
   public:
    LegacyProfilingPass() : ModulePass(ID) {}
    virtual bool runOnModule(Module &M);
    static char ID;

   private:
    void addEndProfilingFuncCall(Module &M);
   private:
     FunctionCallee getLLFILibProfilingFunc(Module &M);
     FunctionCallee getLLFILibEndProfilingFunc(Module &M);
  };

  // For new PM
  struct ProfilingPass:  llvm::PassInfoMixin<ProfilingPass> {
    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &){

      auto obj = new LegacyProfilingPass();
      bool isChanged = obj->runOnModule(M);

      delete obj;
      return (isChanged) ? llvm::PreservedAnalyses::none():
                           llvm::PreservedAnalyses::all();
    }

    // Without isRequired returning true, this pass will be skipped for functions
    // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
    // all functions with optnone.
    static bool isRequired() { return true; }
  };
}
#endif
