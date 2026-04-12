#include "FICustomSelectorManager.h"
#include "FIInstSelector.h"

#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace llfi {

/**
 * This sample instruction selector only selects instructions in function main
 */
// TODO: enable custom selctor to have more sources of options, e.g. read from
// config file
class SampleFIInstSelector : public HardwareFIInstSelector {
private:
  bool isInstFITarget(Instruction* inst) override {
    if (inst->getParent()->getParent()->getName() == "main")
      return true;
    else
      return false;
  }

public:
  void getCompileTimeInfo(std::map<std::string, std::string>& info) override {
    info["failure_class"] = "HardwareFault";
    info["failure_mode"] = "OnlyMain";
    info["targets"] = "<instructions in main() function>";
    info["injector"] = "<fi_type>";
  }
};

static RegisterFIInstSelector X("onlymain", new SampleFIInstSelector());
} // namespace llfi
