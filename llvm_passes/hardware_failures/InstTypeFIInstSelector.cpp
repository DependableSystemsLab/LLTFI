#include "InstTypeFIInstSelector.h"

#include "llvm/IR/Instructions.h"

namespace llfi {
bool InstTypeFIInstSelector::isInstFITarget(Instruction* inst) {
  unsigned opcode = inst->getOpcode();
  if (opcodelist->find(opcode) != opcodelist->end()) {
    return true;
  }
  return false;
}

} // namespace llfi
