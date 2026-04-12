#include "FICustomSelectorManager.h"
#include "FIRegSelector.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

namespace llfi {

/**
 * This sample register selector only selects constant int as target
 */
class SampleFIRegSelector : public HardwareFIRegSelector {
private:
  bool isRegofInstFITarget(Value* reg, Instruction* inst) override {
    if (isa<ConstantInt>(reg))
      return true;
    else
      return false;
  }
};

static RegisterFIRegSelector X("onlyconstint", new SampleFIRegSelector());

} // namespace llfi
