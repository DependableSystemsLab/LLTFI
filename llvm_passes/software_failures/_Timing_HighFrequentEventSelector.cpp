#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"

#include "FIInstSelector.h"
#include "FICustomSelectorManager.h"
#include "_SoftwareFaultRegSelectors.h"

#include <fstream>
#include <iostream>
#include <set>
#include <string>

using namespace llvm;
namespace llfi {
    class _Timing_HighFrequentEventInstSelector : public SoftwareFIInstSelector {
        public:
        _Timing_HighFrequentEventInstSelector() {
            if (funcNames.empty()) {
                funcNames.insert(std::string("fread"));
                funcNames.insert(std::string("fopen"));
                funcNames.insert(std::string("fwrite"));
            }
        }
            
        void getCompileTimeInfo(std::map<std::string, std::string>& info) override {
            info["failure_class"] = "Timing";
            info["failure_mode"] = "HighFrequentEvent";
            for(std::set<std::string>::iterator SI = funcNames.begin(); SI != funcNames.end(); SI++) {
                info["targets"] += *SI + "()/";
            }
            info["targets"] += "return";
            info["injector"] = "SleepInjector";
        }

        private:
        static std::set<std::string> funcNames;

        bool isInstFITarget(Instruction* inst) override {
            if (isa<CallInst>(inst)) {
                CallInst* CI = cast<CallInst>(inst);
                Function* called_func = CI->getCalledFunction();
                if (called_func == nullptr) {
                    return false;
                }
                std::string func_name = called_func->getName().str();
                if (funcNames.find(func_name) != funcNames.end()) {
                    return true;
                } else {
                    return false;
                }
            } else {
                return isa<ReturnInst>(inst);
            }
        }
    };
    
    std::set<std::string> _Timing_HighFrequentEventInstSelector::funcNames;

    class _Timing_HighFrequentEventRegSelector : public SoftwareFIRegSelector {
        private:
        bool isRegofInstFITarget(Value *reg, Instruction *inst) override {
            if (isa<CallInst>(inst)) {
                CallInst* CI = cast<CallInst>(inst);
                Function* called_func = CI->getCalledFunction();
                if (called_func == nullptr) {
                    return false;
                }
                return reg == CI; // selects dst register
            } else if (isa<ReturnInst>(inst)) {
                ReturnInst* RI = cast<ReturnInst>(inst);
                return reg == RI->getReturnValue();
            } else {
                return false;
            }
        }
    };
    
    static RegisterFIInstSelector A("HighFrequentEvent(Timing)", new _Timing_HighFrequentEventInstSelector());
    static RegisterFIRegSelector B("HighFrequentEvent(Timing)", new _Timing_HighFrequentEventRegSelector());
}
