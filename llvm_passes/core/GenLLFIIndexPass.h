#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace llfi{

  bool runOnModuleMain(Module&);

  // For new PM
  struct GenLLFIIndexPass: llvm::PassInfoMixin<GenLLFIIndexPass> {
    PreservedAnalyses run(llvm::Module &M,
                          llvm::ModuleAnalysisManager &) {
      runOnModuleMain(M);
      return PreservedAnalyses::none();
    }

    // Without isRequired returning true, this pass will be skipped for functions
    // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
    // all functions with optnone.
    static bool isRequired() { return true; }
  };

  // For legacy PM
  class LegacyGenLLFIIndexPass: public ModulePass {
   public:
    LegacyGenLLFIIndexPass() : ModulePass(ID) {}
    virtual bool runOnModule(Module &M);
    static char ID;
  };
}
