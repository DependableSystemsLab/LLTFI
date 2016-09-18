// DO NOT MODIFY!
// File generated on 2016/09/13 14:40:41 PDT

// This file was generated from <LLFI_SRC_ROOT>/tools/FIDL/TargetMultiSourceTemplate.cpp
// by the <LLFI_SRC_ROOT>/tools/FIDL/FIDL-Algorithm.py
// See https://github.com/DependableSystemsLab/LLFI/wiki/Using-FIDL-to-create-a-Custom-Software-Fault-Injector-and-a-Custom-Instruction-Selector
// for more information.

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"

#include "Utils.h"
#include "FIInstSelector.h"
#include "FICustomSelectorManager.h"
#include "_SoftwareFaultRegSelectors.h"

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>

using namespace llvm;
namespace llfi {
//fidl_1
class _API_WrongAPIInstSelector : public SoftwareFIInstSelector {
    public:
//fidl_2
    _API_WrongAPIInstSelector () {
        if (funcNamesTargetArgs.size() == 0) {
//fidl_3
            funcNamesTargetArgs["fgetc"] = std::set<int>();
            funcNamesTargetArgs["fgetc"].insert(2);
            funcNamesTargetArgs["fread"] = std::set<int>();
            funcNamesTargetArgs["fread"].insert(3);
            funcNamesTargetArgs["fopen"] = std::set<int>();
            funcNamesTargetArgs["fopen"].insert(1);
            funcNamesTargetArgs["fopen"].insert(0);
            funcNamesTargetArgs["fwrite"] = std::set<int>();
            funcNamesTargetArgs["fwrite"].insert(3);
        }
    }
    
    virtual void getCompileTimeInfo(std::map<std::string, std::string>& info) {
//fidl_4
        info["failure_class"] = "API";
        info["failure_mode"] = "WrongAPI";
        for (std::map<std::string, std::set<int> >::iterator MI = funcNamesTargetArgs.begin(); MI != funcNamesTargetArgs.end(); MI++) {
            info["targets"] += MI->first + "()/";
        }
        //remove the '/' at the end
        info["targets"] = info["targets"].substr(0, info["targets"].length() - 1);
//fidl_5
        info["injector"] = "BitCorruptionInjector";
    }
    
    static bool isTarget(CallInst* CI, Value* T) {
        std::string func_name = CI->getCalledFunction()->getName();
//fidl_6
        if (funcNamesTargetArgs.find(func_name) == funcNamesTargetArgs.end()) {
            return false;
        }
        for (std::set<int>::iterator SI = funcNamesTargetArgs[func_name].begin(); SI != funcNamesTargetArgs[func_name].end(); SI++) {
            if (*SI >= CI->getNumArgOperands()) {
                continue;
            } else if (T == CI->getArgOperand(*SI)) {
                return true; 
            }
        }
        return false;
    }
    
    private:
    static std::map<std::string, std::set<int> > funcNamesTargetArgs;
    
    virtual bool isInstFITarget(Instruction* inst) {
        if (!isa<CallInst>(inst)) {
            return false;
        }
        
        CallInst* CI = dyn_cast<CallInst>(inst);
        Function* called_func = CI->getCalledFunction();
        if (called_func == NULL) { 
            return false;
        }
        
        std::string func_name = std::string(called_func->getName());
//fidl_7
        return funcNamesTargetArgs.find(func_name) != funcNamesTargetArgs.end();
    }
    
    static bool isTargetLLFIIndex(Instruction* inst) {
//fidl_8
        const long n = 0;
        const long targeted_indices[] = {};
        if (n > 0) {
            long llfiindex = getLLFIIndexofInst(inst);
            for (int i = 0; i < n; i++) {  
                if (llfiindex == targeted_indices[i]) { 
                    return true;
                }
            }
            return false;
        } else {
            return true;
        }
    }
    
    // does something in funcNamesTargetArgs matches partially with func_name?
    static std::map<std::string, std::set<int> >::iterator key_partially_matches(std::string func_name) {
        std::map<std::string, std::set<int> >::iterator SI;
        for (SI = funcNamesTargetArgs.begin(); SI != funcNamesTargetArgs.end(); SI++) {
            if (func_name.find(SI->first) != std::string::npos) {   
                break;
            }
        }
        return SI;
    }
};

//fidl_9
std::map<std::string, std::set<int> >  _API_WrongAPIInstSelector::funcNamesTargetArgs;

class _API_WrongAPIRegSelector: public SoftwareFIRegSelector {
    private:
    virtual bool isRegofInstFITarget(Value *reg, Instruction *inst) {
        if (isa<CallInst>(inst) == false) {
            return false;
        }
        CallInst* CI = dyn_cast<CallInst>(inst);
        Function* called_func = CI->getCalledFunction();
        if (called_func == NULL) {
            return false;
        }
//fidl_10
        if (_API_WrongAPIInstSelector::isTarget(CI, reg)) {
            return true;
        } else { 
            return false;
        }
    }
};

static RegisterFIInstSelector A("WrongAPI(API)", new _API_WrongAPIInstSelector());
static RegisterFIRegSelector B("WrongAPI(API)", new _API_WrongAPIRegSelector());

}

