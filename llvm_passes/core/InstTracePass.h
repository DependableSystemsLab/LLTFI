#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

namespace llfi {

  struct InstTrace : public FunctionPass {

    static char ID;

    InstTrace() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &M) {
      return false;
    }

    virtual bool doFinalization(Module &M);

    long fetchLLFIInstructionID(Instruction *targetInst);

    Instruction* getInsertPoint(Instruction* llfiIndexedInst);

    virtual bool runOnFunction(Function &F);
  };

  struct NewInstTrace:  llvm::PassInfoMixin<NewInstTrace> {
    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &){

      InstTrace tempObj;
      tempObj.doInitialization(M);

      for (Function &F : M) {
        tempObj.runOnFunction(F);
      }

      tempObj.doFinalization(M);

      return PreservedAnalyses::none();
    }

    // Without isRequired returning true, this pass will be skipped for functions
    // decorated with the optnone LLVM attribute. Note that clang -O0 decorates 
    // all functions with optnone.
    static bool isRequired() { return true; }
  };
}//namespace llfi

