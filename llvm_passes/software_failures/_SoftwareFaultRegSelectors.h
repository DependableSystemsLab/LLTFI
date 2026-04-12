#ifndef SOFTWARE_FAULT_REG_SELECTORS_H
#define SOFTWARE_FAULT_REG_SELECTORS_H

#include "llvm/IR/Value.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "FIInstSelector.h"
#include "FIRegSelector.h"
#include "FICustomSelectorManager.h"

#include "llvm/IR/IntrinsicInst.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace llfi {
	class FuncArgRegSelector: public SoftwareFIRegSelector {
	public:
		FuncArgRegSelector(int target_arg) : pos_argument(target_arg), specified_arg(true) {};
		FuncArgRegSelector():pos_argument(0), specified_arg(false) {};
	private:
		int pos_argument;
		bool specified_arg;
		bool isRegofInstFITarget(Value *reg, Instruction *inst) override;
		bool isRegofInstFITarget(Value* reg, Instruction* inst, int pos) override;
	};

	class FuncDestRegSelector: public SoftwareFIRegSelector {
	private:
		bool isRegofInstFITarget(Value *reg, Instruction *inst) override;

	};

	class RetValRegSelector: public SoftwareFIRegSelector {
	private:
		bool isRegofInstFITarget(Value *reg, Instruction *inst) override;

	};

}

#endif // SOFTWARE_FAULT_REG_SELECTORS_H

