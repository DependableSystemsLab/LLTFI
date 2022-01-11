#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"

#include "FIInstSelector.h"
#include "FICustomSelectorManager.h"
#include "Utils.h"

#include "FICustomSelectorManager.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <cassert>

using namespace llvm;

namespace llfi {


static cl::list< std::string > layerNo("layerNo",
         cl::desc("Layer Number in which you want to inject bitflip faults. Pass 0 for injecting faults in all the layers.\n Semi-colon seperated values. Example: 1;0;2"),
         cl::ZeroOrMore);

static cl::list< std::string > layerName("layerName",
         cl::desc("Layer Name in which you want to inject bitflip faults. Semi-colon seperated values. Example: Conv;Relu;Pool"),
         cl::ZeroOrMore);


// Return an array our of string of comma-seperated values.
std::vector<std::string> GetCommaSeperateVals(std::string inp) {

    std::string s = inp;
    std::string delimiter = ";";
    std::vector<std::string> retval;
    size_t pos = 0;

    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        retval.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    
    if ((pos = s.find(delimiter)) == std::string::npos) {
    	
	 retval.push_back(s);
    }
    
    return retval;
}


/**
 * This sample instruction selector only selects instructions in function
     main_graph and belonging to the specified tensor operator.
 */
class CustomTensorOperatorInstSelector : public HardwareFIInstSelector {

public:
    // Data structure to keep track of every Tensor Operator.
    struct Operator {

        std::string OperatorName;
        // ONNX mlir assigns a unique ID to every operator.
        int64_t OperatorNumber;
        // Number of times we have seen this operator.
        int OperatorCount;
        // Operator number to do FI
        int FIOperatorCount;

        Operator(std::string name, std::string count) {

            OperatorName = name;
            FIOperatorCount = atoll(count.c_str());
            OperatorCount = 0;
            OperatorNumber = GetOperatorNumber(name);

            if (OperatorNumber == -1) {
                std::cout<<"Operator name "<< OperatorName.c_str() <<"not found.\n";
                std::cout<<"Please use the following operator name(s): conv, relu, maxpool, matmul, add, avgpool, all, and softmax.";
                assert(false && "Invalid input operator name");
            }

            assert(FIOperatorCount >= 0 && "Invalid input FI operator number");
        }

        // Get unique Id corresponding to the ONNX operator.
        static int64_t GetOperatorNumber(std::string name) {

            char opname[100];
            std::transform(name.begin(), name.end(), name.begin(),
                            [](unsigned char c){ return std::tolower(c); });

	    strcpy(opname, name.c_str());
            
	    if (!strcmp(opname, "conv")) {
                return 1986948931;
            }
            else if (!strcmp(opname, "relu")) {
                return 1970038098;
            }
            else if (!strcmp(opname, "maxpool")) {
                return 30521821366870349;
            }
            else if (!strcmp(opname, "matmul")) {
                return 119251066446157;
            }
            else if (!strcmp(opname, "add")) {
                return 6579265;
            }
            else if (!strcmp(opname, "avgpool")) {
                return 30521821365761601;
            }
            else if (!strcmp(opname, "softmax")) {
                return 33884119937478483;
            }

            return -1;
        }

        bool doFaultInjection(){

            OperatorCount++;

            // Inject fault in the user-specified operator count.
            if (FIOperatorCount == 0 || FIOperatorCount == OperatorCount) return true;

            return false;
        }
    }; // End of struct Operator.

private:
    bool isCustomTensorOperator;
    std::unordered_map<int64_t, std::vector<Operator*>> map;
    bool injectInAll;

    // Add Metadata to LLVM instructions; Only for debugging purposes!
    void addMetadata(llvm::Instruction *ins, char *st = NULL){
         LLVMContext& C = ins->getContext();
         MDNode* N = MDNode::get(C, MDString::get(C, (!st) ? "t" : st));
         ins->setMetadata("Debug", N);
    }

    // Initializes Layer name and number
    void InitializeLayerNameAndNumber(std::string layerNo, std::string layerName) {

        std::vector<std::string> OperatorNames = GetCommaSeperateVals(layerName);
        std::vector<std::string> OperatorNumbers = GetCommaSeperateVals(layerNo);

        assert(OperatorNumbers.size() == OperatorNames.size() && "Number of CSVs given to the layerNo and layerName should be equal.");

	std::cout<<"Initializing"<<std::endl;
        for (int i  = 0; i < OperatorNames.size(); i++) {

            std::string name = OperatorNames[i];
            std::string number = OperatorNumbers[i];

	    std::cout<<"Got operator name="<<name.c_str()<<"   Number="<<number.c_str()<<std::endl;

            // Inject in all operators.
            if (strcmp(name.c_str(), "all") == 0 || strcmp(name.c_str(), "All") == 0) {
                injectInAll = true;
                break;
            }

            int64_t code = Operator::GetOperatorNumber(name);

            // if this operator is already in the map
            if (map.find(code) != map.end()) {
                
                Operator *temp = new Operator(name, number);
                map[code].push_back(temp);
            }
            else {

                std::vector<Operator*> OpArr;
                Operator *temp = new Operator(name, number);
                OpArr.push_back(temp);
                map.insert(make_pair(code, OpArr));
            }
        }
    }

    bool shouldInjectFault(int64_t number) {

        if (injectInAll) return true;

        // If the operator isn't present in the map.
        if (map.find(number) == map.end()) return false;
        else {

            std::vector<Operator*> temp = map[number];
            bool result;

            for (auto it : temp) {
                result |= it->doFaultInjection();
            }

            return result;
        }
    }

    virtual bool isInstFITarget(Instruction *inst) {
        if (inst->getParent()->getParent()->getName() == "main_graph") {
                
            if (map.size() == 0 && !injectInAll){
                InitializeLayerNameAndNumber(layerNo[0], layerName[0]);
            }

            if (inst->getOpcode() == Instruction::Call){
                CallInst* callinst = dyn_cast<CallInst>(inst);

                // If this is OMInstrument function?
                if ((callinst->getCalledFunction())->getName() == "OMInstrumentPoint") {
                    
                    Value* arg1 = callinst->getArgOperand(0); 
                    Value* arg2 = callinst->getArgOperand(1);

                    ConstantInt* ci1 = dyn_cast<ConstantInt>(arg1); 
                    ConstantInt* ci2 = dyn_cast<ConstantInt>(arg2);

                    int64_t argValue1 = ci1->getSExtValue();
                    int64_t argValue2 = ci2->getSExtValue();

                    if (argValue2 == 2 && shouldInjectFault(argValue1)) {
                        
                        // I'm gonna inject fault!
                        isCustomTensorOperator = true;
                    }
                    
                    if (argValue2 == 1) {
                        
                        // Set this to false after the operator ends.
                        isCustomTensorOperator = false;
                    }

                    // std::cout<<ci1->getSExtValue()<<" : "<<ci2->getSExtValue()<<std::endl; 
                }
            }

            if (!isCustomTensorOperator) return false;

            // Injecting fault.
            if (inst->getOpcode() == Instruction::FAdd ||
                inst->getOpcode() == Instruction::FSub ||
                inst->getOpcode() == Instruction::FMul ||
                inst->getOpcode() == Instruction::FDiv ||
                inst->getOpcode() == Instruction::FCmp) {

                addMetadata(inst, "Injected fault");
                std::cout<<"In Custom Tensor Operator pass "<<inst->getOpcodeName()<<std::endl;
                return true;
            }
            
            return false; // Inject Fault in all instructions
        }
        return false;
    }

public:
    CustomTensorOperatorInstSelector(){
        isCustomTensorOperator = false;
        injectInAll = false;
    }

    virtual void getCompileTimeInfo(std::map<std::string, std::string> &info) {
        info["failure_class"] = "HardwareFault";
        info["failure_mode"] = "CustomTensorOperator";
        info["targets"] = "<instructions in main_graph() function and within the specified tensor operator>";
        info["injector"] = "<fi_type>";
    }
};

static RegisterFIInstSelector X("CustomTensorOperator", new CustomTensorOperatorInstSelector());
} // namespace llfi
