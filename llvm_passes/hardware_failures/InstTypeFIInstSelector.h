#ifndef INST_TYPE_FI_INST_SELECTOR_H
#define INST_TYPE_FI_INST_SELECTOR_H
#include "FIInstSelector.h"

#include <set>

using namespace llvm;
namespace llfi {

class InstTypeFIInstSelector : public HardwareFIInstSelector {
public:
  InstTypeFIInstSelector(std::set<unsigned>* opcodelist) {
    this->opcodelist = opcodelist;
  }
  ~InstTypeFIInstSelector() override { delete opcodelist; }
  void getCompileTimeInfo(std::map<std::string, std::string>& info) override {
    info["failure_class"] = "HardwareFault";
    info["failure_mode"] = "SpecifiedInstructionTypes";
    info["targets"] = "<include list in yaml>";
    info["injector"] = "<fi_type>";
  }

private:
  bool isInstFITarget(Instruction* inst) override;

private:
  std::set<unsigned>* opcodelist;
};

} // namespace llfi

#endif
