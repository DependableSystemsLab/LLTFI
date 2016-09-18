#include "inttypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include "Utils.h"

using namespace llvm;
namespace llfi {
class GenLLFIIndexPass : public ModulePass {
public:
  GenLLFIIndexPass() : ModulePass(ID) {}
  bool runOnModule(Module &M) override;
  static char ID;
};

char GenLLFIIndexPass::ID = 0;
static RegisterPass<GenLLFIIndexPass>
    X("genllfiindexpass", "Generate a unique LLFI index for each instruction",
      false, false);

bool GenLLFIIndexPass::runOnModule(Module &M) {
  Instruction *currinst = nullptr;
  uint64_t MaxIndex = 0;

  for (Module::iterator m_it = M.begin(); m_it != M.end(); ++m_it) {
    if (!m_it->isDeclaration()) {
      // m_it is a function
      for (inst_iterator f_it = inst_begin(*m_it); f_it != inst_end(*m_it);
           ++f_it) {
        currinst = &(*f_it);
        if (!dyn_cast<DbgDeclareInst>(currinst)) {
          // dbgs() << *currinst << "\n";
          MaxIndex = setLLFIIndexofInst(currinst);
        }
      }
    }
  }

  if (MaxIndex) {
    FILE *outputFile = fopen("llfi.stat.totalindex.txt", "w");
    if (outputFile)
      fprintf(outputFile, "totalindex=%" PRIu64 "\n", MaxIndex);

    fclose(outputFile);
  }

  return true;
}
}
