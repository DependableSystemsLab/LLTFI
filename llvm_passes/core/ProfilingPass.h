// This pass is run after the transform pass for inserting hooks for fault
// injection
#ifndef PROFILING_PASS_H
#define PROFILING_PASS_H

#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

using namespace llvm;
namespace llfi {
class ProfilingPass : public ModulePass {
public:
  ProfilingPass() : ModulePass(ID) {}
  bool runOnModule(Module &M) override;
  static char ID;

private:
  void addEndProfilingFuncCall(Module &M);

private:
  Constant *getLLFILibProfilingFunc(Module &M);
  Constant *getLLFILibEndProfilingFunc(Module &M);
};

char ProfilingPass::ID = 0;
}

#endif
