
#include "FuncNameFIInstSelector.h"

#include "Utils.h"

#include "llvm/IR/Instructions.h"

namespace llfi {

bool FuncNameFIInstSelector::isInstFITarget(Instruction* inst) {
  std::string func = inst->getParent()->getParent()->getName().str();
  func = demangleFuncName(func);

  if (funclist->find(func) != funclist->end()) {
    return true;
  }
  return false;
}

} // namespace llfi
