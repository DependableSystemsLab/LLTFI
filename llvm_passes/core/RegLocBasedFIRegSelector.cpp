#include "RegLocBasedFIRegSelector.h"

namespace llfi {

bool RegLocBasedFIRegSelector::isRegofInstFITarget(Value *reg, 
                                                          Instruction *inst) {
  if (firegloc == dstreg) {
    return reg == inst;
  } else if (firegloc == allsrcreg) {
    if(isa<GetElementPtrInst>(inst)){
      if(inst->getOperand(inst->getNumOperands()-1) == reg && isa<Constant>(reg)) return false;
    }
    return reg != inst;
  } else if (firegloc == allreg) {
       // dbgs() << "Choosing all regs" << "\n";
       return true;
  } else {
    unsigned srcindex = (unsigned) (firegloc - srcreg1);
    unsigned totalsrcregnum = inst->getNumOperands();
    if (srcindex < totalsrcregnum) {
      if(isa<GetElementPtrInst>(inst)){
        if(inst->getOperand(totalsrcregnum-1) == reg && isa<Constant>(reg)) return false;
      }
      return inst->getOperand(srcindex) == reg;
    } else
      return false;
  }
}

bool RegLocBasedFIRegSelector::isRegofInstFITarget(Value *reg, 
                                                          Instruction *inst,
                                                          int pos) {
  bool result =  isRegofInstFITarget(reg, inst);
  // Only check position if it's not allsrcreg, dstreg or all reg
  if (! (firegloc == allsrcreg || firegloc == dstreg || firegloc == allreg) ) 
	  result = result && (firegloc - srcreg1) == pos;
  return result;
}

}
