#define DEBUG_TYPE "InstructionDuplicationPass"

#include "FICustomSelectorManager.h"
#include "Utils.h"
#include "FIInstSelectorManager.h"
#include "FIInstSelector.h"
#include "InstTypeFIInstSelector.h"
#include "FuncNameFIInstSelector.h"
#include "FIRegSelector.h"
#include "RegLocBasedFIRegSelector.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cassert>

using namespace llvm;
using namespace std;

namespace SID {

    static cl::opt< string > llfiIndex("llfiIndex", cl::desc(" \
        llfiIndex of the arithmetic instruction to duplicate. Default:all"),
        cl::init("all"));

    static cl::opt< string > layerName("operatorName", cl::desc("Name of \
        operator(s) to duplicate. Semi-colon seperated values.\
        Example: conv;relu;matmul;maxpool;all"), cl::init("all"));

    static cl::opt< bool > enableChainDuplication("enableChainDuplication",
        cl::desc("Boolean value to indicate whether to do arithmetic chain \
            duplication or not. Default: False"), cl::init(false));

    // Return an array our of string of comma-seperated values.
    vector<string> getCommaSeperateVals(string inp) {

        string s = inp;
        string delimiter = ";";
        vector<string> retval;
        size_t pos = 0;

        string token;
        while ((pos = s.find(delimiter)) != string::npos) {
            token = s.substr(0, pos);
            retval.push_back(token);
            s.erase(0, pos + delimiter.length());
        }

        if ((pos = s.find(delimiter)) == string::npos) {
            retval.push_back(s);
        }

        return retval;
    }

    // Get unique Id corresponding to the ONNX operator.
    static int64_t getOperatorNumber(string name) {

        char opname[100];
        transform(name.begin(), name.end(), name.begin(),
                        [](unsigned char c){ return tolower(c); });

        strcpy(opname, name.c_str());

        // ONNX assigns unique IDs to each tensor operator.
        map<string, int64_t> ONNXOperatorId = {
            {"conv", 1986948931},
            {"relu", 1970038098},
            {"maxpool", 30521821366870349},
            {"matmul", 119251066446157},
            {"add", 6579265},
            {"avgpool", 30521821365761601},
            {"softmax", 33884119937478483}
        };

        if (ONNXOperatorId.find(opname) == ONNXOperatorId.end())
            return -1;

        return ONNXOperatorId[opname];
    }

    // Add Metadata to LLVM instructions; Only for debugging purposes!
    void addMetadata(Instruction *ins, char *st = NULL){
         LLVMContext& C = ins->getContext();
         MDNode* N = MDNode::get(C, MDString::get(C, (!st) ? "t" : st));

         char finalMD[1000] = "Debug.";
         strcat(finalMD, st);
         ins->setMetadata(finalMD, N);
    }

    void printBB(BasicBlock* bb){

        errs()<<"------- Printing BB -------------\n";
        for (BasicBlock::const_iterator i = bb->begin(); i != bb->end(); ++i) {

            Instruction* inst = const_cast<llvm::Instruction*>(&*i);
            errs()<<*inst<<"\n";
        }
    }

    void printFunction(Function& F){
         errs()<<"------- Printing Function -------------\n";

        for (BasicBlock& bb : F){
            for (BasicBlock::const_iterator i = bb.begin(); i != bb.end(); ++i) {

                Instruction* inst = const_cast<llvm::Instruction*>(&*i);
                errs()<<*inst<<"\n";
            }
        }
    }
}

using namespace SID;

namespace llfi{

    class InstructionDuplicationPass: public FunctionPass
    {

    private:
        bool isInitialized;
        vector<int64_t> operatorValues;
        bool injectInAllOperators;
        vector<int64_t> llfiIndexes;
        bool injectInAllIndexes;
        bool isChainDuplication;

        // Initializes Layer name and granularity of instruction duplication.
        void initializeGranularityAndLayerName(string llfiIndex,
                string layerName, bool isChainDupl) {

            // Parse operators.
            vector<string> OperatorNames = getCommaSeperateVals(layerName);

            for (string name : OperatorNames) {

                if (name.find("all") != string::npos) {
                    injectInAllOperators = true;
                    break;
                }

                int64_t temp = getOperatorNumber(name);

                if (temp == -1) {
                    cerr<<"Invalid operator name: "<<name<<"\n";
                    assert(false && "Invalid Operator Name");
                }

                operatorValues.push_back(temp);
            }

            // Parse enableChainDuplication boolean
            isChainDuplication = isChainDupl;

            // Parse LLFIIndexes for FI.
            vector<string> LLFIIndexes = getCommaSeperateVals(llfiIndex);

            for (string index : LLFIIndexes) {

                if (index.find("all") != string::npos) {
                    injectInAllIndexes = true;
                    break;
                }

                int64_t indexLong = atol(index.c_str());

                assert(indexLong > 0 && "Invalid LLFIIndex");

                llfiIndexes.push_back(indexLong);
            }
        }

    public:

        static char ID;

        InstructionDuplicationPass():FunctionPass(ID)
        {
            isInitialized = false;
            injectInAllIndexes = false;
            injectInAllOperators = false;
        }

        // Duplicate a single arithmetic instruction
        void duplicateInstruction(Instruction* inst) {

            // Clone the instruction and reassign the operands.
            Instruction* duplicatedInst = inst->clone();
            for(unsigned int i = 0; i < duplicatedInst->getNumOperands(); i++){
                duplicatedInst->setOperand(i, inst->getOperand(i));
            }

            // Copy metadata
            MDNode *mdnode = inst->getMetadata("llfi_index");
            inst->setMetadata("llfi_index", NULL);
            duplicatedInst->setMetadata("llfi_index", mdnode);
            addMetadata(duplicatedInst, "Duplicated_Instruction");

            // Insert the duplicate instruction
            duplicatedInst->insertBefore(inst->getNextNode());

            IRBuilder<> IRB(inst->getParent());
            IRB.SetInsertPoint(duplicatedInst->getNextNode());

            auto Fn = inst->getFunction()->getParent()->getOrInsertFunction(
                    "compareFloatValues", Type::getFloatTy(inst->getContext()),
                    Type::getFloatTy(inst->getContext()),
                    Type::getFloatTy(inst->getContext()));

            Value *funret = IRB.CreateCall(Fn, {inst, duplicatedInst});

            auto myIf = [&](Use &operand) {
                if (isa<CallInst>(operand.getUser()))
                    return false;
                return true;
            };

            inst->replaceUsesWithIf(funret, myIf);
        }

        // Duplicate a chain of arithmetic instructions.
        void duplicateInstructionChain(vector<Instruction*> insVector)
        {

            llvm::ValueToValueMapTy vmap;
            vector<Instruction*> new_instructions;

            // Keep track of the last instructions of the instruction chain.
            Instruction* lastInst, *lastInstDupl;

            if (insVector.size() > 0) {
                for (auto *inst: insVector) {

                    // Clone the instruction
                    Instruction* duplicatedInst = inst->clone();

                    lastInst = inst;
                    lastInstDupl = duplicatedInst;

                    for(unsigned int i = 0; i < duplicatedInst->getNumOperands(); i++){
                        duplicatedInst->setOperand(i, inst->getOperand(i));
                    }

                    // Copy metadata
                    MDNode *mdnode = inst->getMetadata("llfi_index");
                    inst->setMetadata("llfi_index", NULL);
                    duplicatedInst->setMetadata("llfi_index", mdnode);
                    addMetadata(duplicatedInst, "Duplicated_Instruction_In_Chain");

                    // Insert the duplicated instruction in the LLVM IR
                    duplicatedInst->insertAfter(inst);

                    new_instructions.push_back(duplicatedInst);
                    vmap[inst] = duplicatedInst;
                }
            }

            for (auto *i : new_instructions) {
                llvm::RemapInstruction(i, vmap, RF_NoModuleLevelChanges |
                    RF_IgnoreMissingLocals);
            }

            // Insert the compareFloatValue function
            IRBuilder<> IRB(lastInst->getParent());
            IRB.SetInsertPoint(lastInstDupl->getNextNode());

            auto Fn = lastInst->getFunction()->getParent()->
                        getOrInsertFunction("compareFloatValues",
                            Type::getFloatTy(lastInst->getContext()),
                            Type::getFloatTy(lastInst->getContext()),
                            Type::getFloatTy(lastInst->getContext())
                            );

            Value *funret = IRB.CreateCall(Fn, {lastInst, lastInstDupl});

            // Replace all use of the arithmatic instruction with the function
            // return value
            auto myIf = [&](Use &operand) {
                if (isa<CallInst>(operand.getUser()))
                    return false;
                return true;
            };

            lastInst->replaceUsesWithIf(funret, myIf);
        }

        bool isArithmeticInstruction(Instruction* inst)
        {
            // Don't do instruction duplication in FCmp.
            if (inst != NULL && (inst->getOpcode() == Instruction::FAdd ||
                        inst->getOpcode() == Instruction::FSub ||
                        inst->getOpcode() == Instruction::FMul ||
                        inst->getOpcode() == Instruction::FDiv))
                return true;
            else
                return false;
        }

        bool checkInstructionIndex(Instruction* inst) {

            if (injectInAllIndexes) return true;

            MDNode *mdnode = inst->getMetadata("llfi_index");
            long vindex = 0;
            if (mdnode) {
                ConstantInt *cns_index = mdconst::dyn_extract<ConstantInt>(mdnode->getOperand(0));
                vindex = cns_index->getSExtValue();
            }

            for(long idx : llfiIndexes) {

                if (idx == vindex)
                    return true;
            }

            return false;
        }

        bool doArithmeticInstructionDuplication(Function& F)
        {
            vector<Instruction*> arithInst;
            bool isCustomTensorOperator = false;

            // Find all the floating-point arithmetic instructions in this function
            for (BasicBlock &bb : F) {
                for (BasicBlock::const_iterator i = bb.begin(); i != bb.end();
                    ++i) {
                    Instruction* inst = const_cast<llvm::Instruction*>(&*i);

                     if (inst->getOpcode() == Instruction::Call){
                        CallInst* callinst = dyn_cast<CallInst>(inst);

                        // If this is OMInstrument function?
                        if ((callinst->getCalledFunction())->getName() ==
                            "OMInstrumentPoint") {

                            Value* arg1 = callinst->getArgOperand(0);
                            Value* arg2 = callinst->getArgOperand(1);

                            ConstantInt* ci1 = dyn_cast<ConstantInt>(arg1);
                            ConstantInt* ci2 = dyn_cast<ConstantInt>(arg2);

                            int64_t argValue1 = ci1->getSExtValue();
                            int64_t argValue2 = ci2->getSExtValue();

                            if (argValue2 == 2 && shouldInjectFault(argValue1)) {

                                // Inject fault!
                                isCustomTensorOperator = true;
                            }

                            if (argValue2 == 1 && shouldInjectFault(argValue1)) {

                                // Set this to false after the operator ends.
                                isCustomTensorOperator = false;
                            }
                        }
                    }

                    if (isCustomTensorOperator && isArithmeticInstruction(inst) && checkInstructionIndex(inst)) {

                        arithInst.push_back(inst);
                    }
                }
            }

            // Then duplicate the arithmetic instructions.
            for (auto ins : arithInst){

                duplicateInstruction(ins);
            }

            return true;
        }

        bool doArithmeticChainDuplication(Function& F)
        {
            vector<vector<Instruction*>> arithInst;
            bool isCustomTensorOperator = false;

            // Find all the floating-point arithmetic instructions in this function
            for (BasicBlock &bb : F) {
                for (BasicBlock::const_iterator i = bb.begin(); i != bb.end();
                    ++i) {
                    Instruction* inst = const_cast<llvm::Instruction*>(&*i);

                     if (inst->getOpcode() == Instruction::Call){
                        CallInst* callinst = dyn_cast<CallInst>(inst);

                        // If this is OMInstrument function?
                        if ((callinst->getCalledFunction())->getName() ==
                            "OMInstrumentPoint") {

                            Value* arg1 = callinst->getArgOperand(0);
                            Value* arg2 = callinst->getArgOperand(1);

                            ConstantInt* ci1 = dyn_cast<ConstantInt>(arg1);
                            ConstantInt* ci2 = dyn_cast<ConstantInt>(arg2);

                            int64_t argValue1 = ci1->getSExtValue();
                            int64_t argValue2 = ci2->getSExtValue();

                            if (argValue2 == 2 && shouldInjectFault(argValue1)) {

                                // Inject fault!
                                isCustomTensorOperator = true;
                            }

                            if (argValue2 == 1 && shouldInjectFault(argValue1)) {

                                // Set this to false after the operator ends.
                                isCustomTensorOperator = false;
                            }
                        }
                    }

                    if (isCustomTensorOperator && isArithmeticInstruction(inst) && checkInstructionIndex(inst)){

                        vector<Instruction*> temp;
                        temp.push_back(inst);

                        // Detects chain of arithmetic instructions
                        Instruction* currInst = inst;
                        while (true) {
                            if (isArithmeticInstruction(currInst->getNextNonDebugInstruction())&&
                                checkInstructionIndex(currInst->getNextNonDebugInstruction())) {
                                ++i;
                                currInst = currInst->getNextNonDebugInstruction();
                                temp.push_back(currInst);
                            }
                            else
                                break;
                        }

                        arithInst.push_back(temp);
                    }
                }
            }

            // Then duplicate the arithmetic instructions.
            for (auto insVector : arithInst){

                if (insVector.size() == 1)
                    duplicateInstruction(insVector[0]);
                else if (insVector.size() > 1){
                    duplicateInstructionChain(insVector);
                }
                else
                    assert(false && "Size of insVector can't be zero!");
            }

            return true;
        }

        bool shouldInjectFault(int64_t number) {

		    if (injectInAllOperators) return true;

		    // If the operator isn't present in the map.
		    for (auto num : operatorValues) {

		        if (number == num)
		            return true;
		    }

		    return false;
	    }

        bool runOnMainGraph(Function& F)
        {
            // Parse input options.
            if (!isInitialized) {
                isInitialized = true;
                initializeGranularityAndLayerName(llfiIndex, layerName, enableChainDuplication);
            }

            if (isChainDuplication){
                return doArithmeticChainDuplication(F);
            }
            else {
                return doArithmeticInstructionDuplication(F);
            }

            return false;
        }

        virtual bool runOnFunction(Function &F)
        {

            if (F.getName() == "main_graph") {
                return runOnMainGraph(F);
            }

            return false;
        }
    };

    char InstructionDuplicationPass::ID = 0;

    static RegisterPass<InstructionDuplicationPass>
        X("InstructionDuplicationPass", "Automatic Duplication of ML applications",
            false, false);
}
