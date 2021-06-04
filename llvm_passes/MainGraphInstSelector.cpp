#include "llvm/IR/Instructions.h"

#include "FICustomSelectorManager.h"
#include "FIInstSelector.h"

using namespace llvm;

namespace llfi {

/**
 * This sample instruction selector only selects instructions in function
   main_graph
 */
// TODO: enable custom selctor to have more sources of options, e.g. read from
// config file
class MainGraphInstSelector : public HardwareFIInstSelector {
private:
  virtual bool isInstFITarget(Instruction *inst) {
    if (inst->getParent()->getParent()->getName() == "main_graph") {
      if (inst->getOpcode() == Instruction::FAdd ||
          inst->getOpcode() == Instruction::FMul ||
          inst->getOpcode() == Instruction::FCmp) {
        return true;
      }
    }
    return false;
  }

public:
  virtual void getCompileTimeInfo(std::map<std::string, std::string> &info) {
    info["failure_class"] = "HardwareFault";
    info["failure_mode"] = "MainGraph";
    info["targets"] = "<instructions in main_graph() function>";
    info["injector"] = "<fi_type>";
  }
};

static RegisterFIInstSelector X("maingraph", new MainGraphInstSelector());
} // namespace llfi
