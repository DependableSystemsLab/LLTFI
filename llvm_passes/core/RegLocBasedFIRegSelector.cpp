// Copyright (C) 2023 Intel Corporation (HDFIT components)
// SPDX-License-Identifier: Apache-2.0

#include "RegLocBasedFIRegSelector.h"

namespace llfi {

// HDFIT: Utility function to identify FP32 (vector) registers
bool isRegFP32(Value* reg) {
	Type* regType = reg->getType();
	if (!regType) {
		return false;
	}

#ifndef HDFIT_DOUBLE
	return regType->getScalarType()->isFloatTy();
#else // HDFIT_DOUBLE
	return regType->getScalarType()->isDoubleTy();
#endif
}
// ---------------------------------------------------------------

bool RegLocBasedFIRegSelector::isRegofInstFITarget(Value *reg, 
                                                          Instruction *inst) {
// HDFIT: FP32 check on the given register
  if (!isRegFP32(reg)) {
	  return false;
  }
// ---------------------------------------------------------------
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
