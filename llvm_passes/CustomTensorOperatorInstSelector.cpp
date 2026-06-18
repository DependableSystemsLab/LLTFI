#include "FICustomSelectorManager.h"
#include "FIInstSelector.h"
#include "Utils.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CommandLine.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

using namespace llvm;

namespace llfi {

static cl::list<std::string> layerNo("layerNo", cl::desc("Layer Number in \
which you want to inject bitflip faults. Pass 0 for injecting faults in all the \
layers.\n Semi-colon seperated values. Example: 1;0;2"),
                                     cl::ZeroOrMore);

static cl::list<std::string> layerName("layerName", cl::desc("Layer Name in \
which you want to inject bitflip faults. Semi-colon seperated values. Example: \
Conv;Relu;Pool"),
                                       cl::ZeroOrMore);

// Return an array our of string of comma-seperated values.
std::vector<std::string> getCommaSeperateVals(std::string inp) {

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

  retval.push_back(s);

  return retval;
}

// Return the ONNX operator name as a string.
std::string extractONNXOperatorName(Value *V) {
  auto *GV = dyn_cast<GlobalVariable>(V);
  Constant *Init = GV->getInitializer();

  auto *CDA = dyn_cast<ConstantDataArray>(Init);
  if (!CDA || !CDA->isString()) {
    return "";
  }

  StringRef onnxOpNameStrRef = CDA->getAsString();
  std::string onnxOpNameStr = onnxOpNameStrRef.str();

  std::transform(onnxOpNameStr.begin(), onnxOpNameStr.end(),
                 onnxOpNameStr.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  std::string onnxOpPrefix = "onnx.";
  size_t pos = onnxOpNameStr.find(onnxOpPrefix);
  std::string result = onnxOpNameStr.substr(pos + onnxOpPrefix.length());
  return result;
}

/**
 * This sample instruction selector only selects instructions in function
 *   main_graph and belonging to the specified tensor operator.
 */
class CustomTensorOperatorInstSelector : public HardwareFIInstSelector {

public:
  // Data structure to keep track of every Tensor Operator.
  struct Operator {

    std::string OperatorName;
    // Number of times we have seen this operator.
    int OperatorCount;
    // Operator number to do FI
    int FIOperatorCount;

    // Check if provided ONNX operator is valid and supported.
    bool isValidOperator(std::string name) {

      std::vector<std::string> ONNXOperators = {
          "conv",    "relu",    "maxpool", "matmul",  "add",
          "avgpool", "softmax", "loop",    "nonmaxs", "unsqueeze"};

      return (std::find(ONNXOperators.begin(), ONNXOperators.end(), name) !=
              ONNXOperators.end());
    }

    Operator(std::string name, std::string count) {

      OperatorName = name;
      FIOperatorCount = (int)atoll(count.c_str());
      OperatorCount = 0;

      if (!isValidOperator(OperatorName)) {
        std::cout << "Operator name " << OperatorName << " not found.\n";
        std::cout << "Please use the following operator name(s):\
                conv, relu, maxpool, matmul, add, avgpool, all, and softmax.";
        assert(false && "Invalid input operator name");
      }

      assert(FIOperatorCount >= 0 && "Invalid input FI operator number");
    }

    bool doFaultInjection() {

      OperatorCount++;

      // Inject fault in the user-specified operator count.
      if (FIOperatorCount == 0 || FIOperatorCount == OperatorCount)
        return true;

      return false;
    }
  }; // End of struct Operator.

private:
  bool isCustomTensorOperator;
  std::unordered_map<std::string, std::vector<Operator *>> map;
  bool injectInAll;
  int64_t instrumentPoint;

  // Add Metadata to LLVM instructions; Only for debugging purposes!
  void addMetadata(llvm::Instruction *ins, const char *st = nullptr) {
    LLVMContext &C = ins->getContext();
    MDNode *N = MDNode::get(C, MDString::get(C, (!st) ? "t" : st));
    ins->setMetadata("Debug", N);
  }

  // Initializes Layer name and number
  void initializeLayerNameAndNumber(std::string layerNo,
                                    std::string layerName) {

    std::vector<std::string> OperatorNames = getCommaSeperateVals(layerName);
    std::vector<std::string> OperatorNumbers = getCommaSeperateVals(layerNo);

    assert(OperatorNumbers.size() == OperatorNames.size() &&
           "Number of CSVs given to the layerNo and layerName should be equal");

    for (int i = 0; i < (int)OperatorNames.size(); i++) {

      const std::string &name = OperatorNames[i];
      const std::string &number = OperatorNumbers[i];

      // Inject in all operators.
      if (strcmp(name.c_str(), "all") == 0 ||
          strcmp(name.c_str(), "All") == 0) {
        injectInAll = true;
        break;
      }

      // if this operator is already in the map
      if (map.find(name) != map.end()) {

        Operator *temp = new Operator(name, number);
        map[name].push_back(temp);
      } else {

        std::vector<Operator *> OpArr;
        Operator *temp = new Operator(name, number);
        OpArr.push_back(temp);
        map.insert(make_pair(name, OpArr));
      }
    }
  }

  bool shouldInjectFault(std::string opName) {

    if (injectInAll)
      return true;

    // If the operator isn't present in the map.
    if (map.find(opName) == map.end())
      return false;
    else {

      std::vector<Operator *> temp = map[opName];
      bool result = false;

      for (auto it : temp) {
        result |= it->doFaultInjection();
      }

      return result;
    }
  }

  bool isInstFITarget(Instruction *inst) override {
    if (inst->getParent()->getParent()->getName().starts_with("main_graph")) {

      if (map.empty() && !injectInAll) {
        initializeLayerNameAndNumber(layerNo[0], layerName[0]);
      }

      if (inst->getOpcode() == Instruction::Call) {
        CallInst *callinst = cast<CallInst>(inst);

        // If this is OMInstrument function?
        if (callinst->getCalledFunction() &&
            callinst->getCalledFunction()->getName() == "OMInstrumentPoint") {

          Value *arg1 = callinst->getArgOperand(0);
          std::string onnxOpName = extractONNXOperatorName(arg1);

          Value *arg2 = callinst->getArgOperand(1);

          ConstantInt *ci = dyn_cast<ConstantInt>(arg2);
          if (onnxOpName == "" || !ci)
            return false;

          int64_t argValue2 = ci->getSExtValue();

          if (instrumentPoint == 0 && shouldInjectFault(onnxOpName)) {

            // Inject fault!
            isCustomTensorOperator = true;
            instrumentPoint = argValue2;
          }

          if (argValue2 == instrumentPoint + 1) {

            // Set this to false after the operator ends.
            isCustomTensorOperator = false;
            instrumentPoint = 0;
          }
        }
      }

      if (!isCustomTensorOperator)
        return false;

      // Injecting fault.
      if (inst->getOpcode() == Instruction::FAdd ||
          inst->getOpcode() == Instruction::FSub ||
          inst->getOpcode() == Instruction::FMul ||
          inst->getOpcode() == Instruction::FDiv ||
          inst->getOpcode() == Instruction::FCmp) {

        addMetadata(inst, "Injected fault");
        return true;
      }

      return false; // Inject Fault in all instructions
    }
    return false;
  }

public:
  CustomTensorOperatorInstSelector() {
    isCustomTensorOperator = false;
    injectInAll = false;
  }

  void getCompileTimeInfo(std::map<std::string, std::string> &info) override {
    info["failure_class"] = "HardwareFault";
    info["failure_mode"] = "CustomTensorOperator";
    info["targets"] = "<instructions in main_graph() function and within \
            the specified tensor operator>";
    info["injector"] = "<fi_type>";
  }
};

static RegisterFIInstSelector X("CustomTensorOperator",
                                new CustomTensorOperatorInstSelector());
} // namespace llfi
