#include "llvm/IR/Function.h"
#include <cstdio>

#include "Utils.h"
#include "GenLLFIIndexPass.h"

using namespace llvm;
namespace llfi {

  // Main functionality of this pass
  bool runOnModuleMain(Module &M) {
    Instruction *currinst;

    for (Module::iterator m_it = M.begin(); m_it != M.end(); ++m_it) {
      if (!m_it->isDeclaration()) {
        // m_it is a function
        for (inst_iterator f_it = inst_begin(&*m_it); f_it != inst_end(&*m_it);
             ++f_it) {
          currinst = &(*f_it);
          setLLFIIndexofInst(currinst);
        }
      }
    }

    if (currinst) {
      long totalindex = getLLFIIndexofInst(currinst);
      FILE *outputFile = fopen("llfi.stat.totalindex.txt", "w");
      if (outputFile)
        fprintf(outputFile, "totalindex=%ld\n", totalindex);

      fclose(outputFile);
    }

    return true;
  }

  // Registration for old PM
  char LegacyGenLLFIIndexPass::ID = 0;
  static RegisterPass<LegacyGenLLFIIndexPass> X(
      "genllfiindexpass", "Generate a unique LLFI index for each instruction",
      false, false);

  bool LegacyGenLLFIIndexPass::runOnModule(Module &M) {
    return runOnModuleMain(M);
  }
}
