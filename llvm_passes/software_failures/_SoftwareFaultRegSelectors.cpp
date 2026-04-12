#include "_SoftwareFaultRegSelectors.h"

using namespace std;
namespace llfi {
    bool FuncArgRegSelector::isRegofInstFITarget(Value *reg, Instruction *inst) {
        if (!isa<CallInst>(inst))
            return false;
        CallInst* CI = cast<CallInst>(inst);
        if (this->specified_arg) {
            return reg == CI->getArgOperand(this->pos_argument);
        } else {
            for (int i = 0; i < (int)CI->arg_size(); i++) {
                if (reg == CI->getArgOperand(i)) return true;
            }
            return false;
        }
    }
    bool FuncArgRegSelector::isRegofInstFITarget(Value *reg, Instruction *inst, int pos){
    	if(specified_arg == true)
	    	return isRegofInstFITarget(reg, inst) && pos == this->pos_argument;
        return false;
    }

    bool FuncDestRegSelector::isRegofInstFITarget(Value *reg, Instruction *inst) {
        if (!isa<CallInst>(inst))
            return false;
        return reg == inst;
    }

    bool RetValRegSelector::isRegofInstFITarget(Value *reg, Instruction *inst) {
        if (!isa<ReturnInst>(inst))
            return false;
        ReturnInst* RI = cast<ReturnInst>(inst);
        return reg == RI->getReturnValue();
    }

    static RegisterFIRegSelector A("FuncArgRegSelector", new FuncArgRegSelector());
    static RegisterFIRegSelector B("RetValRegSelector", new RetValRegSelector());
    static RegisterFIRegSelector C("FuncDestRegSelector", new FuncDestRegSelector());
}

 
