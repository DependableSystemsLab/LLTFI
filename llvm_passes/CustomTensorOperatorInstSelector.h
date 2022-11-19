
using namespace llvm;

namespace llfi {

/**
 * This sample instruction selector only selects instructions in function
 *   main_graph and belonging to the specified tensor operator.
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

        // Get unique Id corresponding to the ONNX operator.
        static int64_t getOperatorNumber(std::string name) {

            char opname[100];
            std::transform(name.begin(), name.end(), name.begin(),
                            [](unsigned char c){ return std::tolower(c); });

            strcpy(opname, name.c_str());

            std::cout<<"OperatorName: "<<opname<<"\n";

            // ONNX assigns unique IDs to each tensor operator.
            std::map<std::string, int64_t> ONNXOperatorId = {
                {"conv", 1986948931},
                {"relu", 1970038098},
                {"maxpool", 30521821366870349},
                {"matmul", 119251066446157},
                {"add", 6579265},
                {"avgpool", 30521821365761601},
                {"softmax", 33884119937478483},
                {"loop", 1886351180},
                {"nonmaxs", 23494782373228366},
                {"unsqueeze", 28540527736745557}
            };

            if (ONNXOperatorId.find(opname) == ONNXOperatorId.end())
                return -1;

            return ONNXOperatorId[opname];
        }

        Operator(std::string name, std::string count) {

            OperatorName = name;
            FIOperatorCount = atoll(count.c_str());
            OperatorCount = 0;
            OperatorNumber = getOperatorNumber(name);

            if (OperatorNumber == -1) {
                std::cout<<"Operator name "<< OperatorName.c_str() <<
                    " not found.\n";
                std::cout<<"Please use the following operator name(s):\
                conv, relu, maxpool, matmul, add, avgpool, all, and softmax.";
                assert(false && "Invalid input operator name");
            }

            assert(FIOperatorCount >= 0 && "Invalid input FI operator number");
        }

        bool doFaultInjection(){

            OperatorCount++;

            // Inject fault in the user-specified operator count.
            if (FIOperatorCount == 0 || FIOperatorCount == OperatorCount)
                return true;

            return false;
        }
    }; // End of struct Operator.

private:
    bool isCustomTensorOperator;
    std::unordered_map<int64_t, std::vector<Operator*>> map;
    bool injectInAll;

    // Add Metadata to LLVM instructions; Only for debugging purposes!
    void addMetadata(llvm::Instruction *ins, char *st);

    // Initializes Layer name and number
    void initializeLayerNameAndNumber(std::string layerNo,
            std::string layerName);

    bool shouldInjectFault(int64_t number);

    virtual bool isInstFITarget(Instruction *inst);

public:
    CustomTensorOperatorInstSelector(){
        isCustomTensorOperator = false;
        injectInAll = false;
    }

    virtual void getCompileTimeInfo(std::map<std::string, std::string> &info) {
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
