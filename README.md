
# ISSRE 2023 Artifacts Evaluation Guide

This README file describes the structure of this project, our fault injection (FI) tool, benchmarks, instructions to build the tool, and, finally, instructions to reproduce the experiments reported in our paper.

## Artifacts Description
### LLTFI - Our FI tool

LLTFI (Low Level Tensor Fault Injector) is a unified SWiFI (Software-implemented fault injection) tool that supports fault injection of both C/C++ programs and ML applications written using high-level frameworks such as TensorFlow and PyTorch. LLTFI supports FI in both vision-based and NLP-based models.

In this work, we modified LLTFI to carry out FI in Large Language Models (LLMs).

#### More Information on LLTFI (Optional - Not Required for AE)
[LLTFI Documentation](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/README_LLTFI.md)\
[LLTFI's Original Paper](https://blogs.ubc.ca/dependablesystemslab/2022/07/29/lltfi-framework-agnostic-fault-injection-for-machine-learning-applications/)

### Benchmarks Used in this Work

Same as Table 1 in the paper. The links in the following table refers to the location of the benchmarks and the benchmark-specific scripts within this project.

| RQ Used In | Benchmark Name   | Link
| ---------- | ----------|--------- |
| 1 & 2 | PubMedBert | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/BioMedNLP) |
| 1 & 2 | CodeBert | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/CodeBert)|
| 1 & 2 | T5-Encoder | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/t5-encoder)|
| 1 & 2 | T5-Decoder | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/t5-decoder)|
 1 & 2 | GPT2 | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/gpt2-lm-head-10)|
  1 & 2 | Roberta | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/roberta-seq-classification-9)|
| 3 | Bert Variants | [Link](https://github.com/DependableSystemsLab/LLTFI/tree/master/sample_programs/ml_sample_programs/nlp_models/BertVariants)


### Project Structure

    ├── bin/                   # LLTFI's python wrappers for profiling and FI
    ├── config/                # CMake configurations
    ├── docker/                # Constains instructions and the docker file for obtaining LLTFI via docker.
    ├── docs/                  # Documentation of different FI configurations that can be used with LLTFI.
    ├── images/                # Images used in README files.
    ├── installer/
        ├── InstallLLTFI.py    # Installation script to install LLTFI and its dependencies.
    ├── llvm_passes/           # LLVM Passes used by LLTFI's core for profiling and FI.
    ├── runtime_lb/            # LLTFI's runtime components.
    ├── sample_programs/       # Large, real-world, C++ End-to-End tests and ML models.
        ├── cpp_sample_programs/ 
        ├── ml_sample_programs/
            ├── nlp_models/    # Contains NLP models.
            ├── vision_models/ # Contains Vision-based ML models.
    ├── test_suite/            # Small C++ End-to-End tests for validating LLTFI's functionality.
    ├── tools/                 # Helper scripts and 3rd party tools used by LLTFI.
    ├── web-app/               # A GUI frontend of LLTFI (now depreciated).
    ├── LICENSE.txt
    ├── CREDITS.txt
    ├── README.md              # README file for ISSRE'23 Artifact Evaluation.
    └── README_LLTFI.md        # README file for instralling and testing LLTFI.
    ...

## Environment Setup
The following are the recommended system configurations required to reproduce our  experiments.\\

**CPU:** Intel I5/I7/I9, or AMD Ryzen Threadripper \
**Operation System:**   Ubuntu 20.04 \
**Endianess:**   Little Endian (must)\
**Bit:**   64-bit \
**RAM:**   atleast 4 GB (Building LLVM requires a decent amount of RAM) \
**SSD/HDD:**   atleast 100 GB (Required to download our benchmarks and run experiments) \
**GPU:** Optional (Could speed-up fine-tuning required for reproducing RQ3)

**Note**:-  To obtain the results presented in the paper, we used a 64-bit AMD Ryzen Threadripper 3960X 24-Core Processor with three NVIDIA RTX A5000 GPUs. However, the good news is that using a different experimental environment **should not affect** the experimental results as long as your environment adheres to the above-mentioned constraints.

## Getting Started

### Installing LLTFI along with its dependencies

There are two ways of building LLTFI and its dependencies: via the auto-installer or through manual installation (for adventure seekers).

#### Auto-Installer
The easiest way to build LLTFI and its dependencies is via the auto-installer (installer/InstallLLTFI.py) for which you do not even need to clone the LLTFI's git repository. Simply download the installer script by itself and copy it into the directory where you would like to build the LLTFI.

Make sure you have the following dependencies:

    CMake (minimum v3.15)
    Python 3 and above
    Ninja >= 1.10.2

Usage:

```$  python3 InstallLLTFI.py``` # Execute the installation script.\
```$  python3 InstallLLTFI.py -h ``` # View different build options for LLTFI.


#### Manual

Refer [LLTFI's README](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/README_LLTFI.md#manual-installation) for manualling installing LLTFI and its dependencies.

### Obtaining Benchmarks

#### For Benchmarks used in RQ1 and RQ2

For PubMedBert and CodeBert, you can use the `getONNXModel.sh` script to obtain the ONNX model of the benchmark. For example, for CodeBert, just execute [getONNXModel.sh script](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/sample_programs/ml_sample_programs/nlp_models/CodeBert/helper_scripts/getONNXModel.sh) in the sample_programs/ml_sample_programs/nlp_models/CodeBert/helper_scripts/ folder.

For T5-encoder, T5-decoder, Roberta, and GPT2, you don't have to explictly obtain the ONNX model. The [compile script](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/sample_programs/ml_sample_programs/nlp_models/gpt2-lm-head-10/compile.sh#L10) will itself download the ONNX model when you try to compile down the model to LLVM IR. The compile script is located in the root folder of every benchmark. For example, for GPT2, it is located in the sample_programs/ml_sample_programs/nlp_models/gpt2-lm-head-10/ folder.

#### For Benchmarks used in R3

Execute the following python script to download and finetine the ONNX models for RQ3:\
https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/tools/fineTuneAndGetONNXFiles.py

**Note:-**  This script will download 15 Bert models and will consume significant amount of SSD/HDD storage. Therefore, execute this script from a folder/disk with sufficient resources.

### Obtaining Inputs of the Benchmarks

To execute the ONNX model with LLTFI, we need to convert the human-readable inputs to the TensorProto (Protobuf) format.

#### Default Inputs that we used in our experiments

For the benchmarks we used in RQ1 and RQ2, the inputs can be found in the 'inputs/' directory within each benchmarks folder. For example, for BioMedNLP, the inputs are in the [sample_programs/ml_sample_programs/nlp_models/BioMedNLP/inputs](https://github.com/DependableSystemsLab/LLTFI/tree/ISSRE23_AE/sample_programs/ml_sample_programs/nlp_models/BioMedNLP/inputs) folder.\
The inputs uses a '.pb' extension which denotes that the inputs are in the TensorProto format. You can use the `exportInputsAsTensors.py` script provided with each benchmark (in the helper_script folder) to convert inputs from human-readable to the TensorProto format.

For the benchmarks we used in RQ3, we used 5 inputs, which can be found in the [fineTuneAndGetONNXFiles.py](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/tools/fineTuneAndGetONNXFiles.py#L28) script.

#### Customizing inputs (Optional - not required for AE)

To customize inputs, you have to just convert them to the TensorProto format. Following is an example of converting custom inputs to the TensorProto format:

```python
from transformers import AutoTokenizer, GPT2Tokenizer
from onnx import numpy_helper
import numpy as np

inputs = []
inputs.append("Your Own custom input")

# Change the tokenizer according to the model.
# For example, for the BioMedNLP model, you can do the following:
#    model_name = "microsoft/BiomedNLP-PubMedBERT-base-uncased-abstract-fulltext"
#    tokenizer = AutoTokenizer.from_pretrained(model_name)
tokenizer = GPT2Tokenizer.from_pretrained('gpt2')

# Tokenize the inputs.
for index in range(0, len(inputs)):
  tokens = np.array(tokenizer.encode(inputs[index])) # Shape : (len,)
  input_arr = tokens.reshape(1,1,-1) # Shape : (1,1,len)
  input_tensor = numpy_helper.from_array(input_arr)
  with open("input_{}.pb".format(index), 'wb') as file:
      file.write(input_tensor.SerializeToString())
```

### Running FI experiments

#### Lowering ONNX models to LLVM IR

To execute ML models with LLTFI, we have to convert them to LLVM IR. For each benchmark, we have provided a `compile.sh` script to automatically lower down the ONNX model to LLVM IR using the ONNX-MLIR tool. To use this script, first export the following environment variables:

```shell
export ONNX_MLIR_SRC=<path to onnx-mlir source>
export ONNX_MLIR_BUILD=<path to where onnx-mlir has been built>
export LLFI_BUILD_ROOT=<path to where LLFI has been built>
export LLVM_DST_ROOT=<path to where LLVM has been built>
```

Now, you can simply execute `bash compile.sh` to generate the LLVM IR file.

#### Configuring FI using input.yaml (Optional - not required for AE)

The following is the default configuration that we used for our experiments:
```shell
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=0
            - -layerName=all

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False

    tracingPropagationOption:
        maxTrace: 250 
        debugTrace: False
        mlTrace: False 
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 1000                    # Number of FI experiments
        fi_type: bitflip                   # Type of hardware fault to inject
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 1                 # Number of faults to inject
        timeOut: 5000
```

For all our benchmarks, this configuration file (`input.yaml`) is already present in their respective folders. For example, for BioMedNLP, the input.yaml file is in the [sample_programs/ml_sample_programs/nlp_models/BioMedNLP/](https://github.com/DependableSystemsLab/LLTFI/blob/master/sample_programs/ml_sample_programs/nlp_models/BioMedNLP/) folder.
Here's a list of all the options you can use to customize the FI: [input_masterlist_ml.yaml](https://github.com/DependableSystemsLab/LLTFI/blob/master/docs/input_masterlist_ml.yaml)

#### Execute!

Before executing, please make sure that the `input.yaml` file is in the same folder from which you will be executing the following commands.

For FI in BioMedNLP:
```shell
rm -rf llfi*

inputSample=0
cp -r "inputs/input$inputSample"_* .

$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe "input${inputSample}_0.pb" "input${inputSample}_1.pb" "input${inputSample}_2.pb" 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe "input${inputSample}_0.pb" "input${inputSample}_1.pb" "input${inputSample}_2.pb" 0
```

Similarly, for other benchmarks, you can find commands to do FI in the 'runALlInputs.sh' or 'runllfiSingleInp.sh' scripts in their respective folders.\
You can find the description of the above-mentioned commands in the following document:
[Get Started with LLTFI Using Command Line](https://github.com/DependableSystemsLab/LLTFI/wiki/Get-Started-with-LLTFI-Using-Command-Line)

### Making Sense of the FI results

After the FI experiment(s) finishes, following files will generated in the same folder:

**llfi/error_output/**: The files under this directory are numbered according to the order of faulty runs and stores the error messages from the corresponding runs. For example, errorfile-run-m-n stores the error message returned from the n th run of the m th runOption: defined in the input.yaml. Common error messages are Program crashed with a return code or Program hang.

**llfi/std_output/**: The files under this directory stores the standard output (STDOUT) of the faulty runs. The names of the files follow the same manner as the files in llfi/error_output.

**llfi/prog_output**: The files under this directory stores the output saved to the disk (filesystem changes) of the faulty runs. The names of the files follow the same manner as the files in llfi/error_output.

**llfi/llfi_stat_output/llfi.stat.fi.injectedfaults._m_-_n_.txt**: These files store important information of the injected fault for each faulty run. The names of these files also follow the same manner as the files in llfi/error_output. Each file contains the statistics of the injected fault of its corresponding faulty run. Here is an example of its content:

```
FI stat: fi_type=bitflip, fi_max_multiple=3, fi_index=1998, fi_cycle=1523447, fi_reg_index=0, fi_reg_pos=2 fi_reg_width=64, fi_bit=47, opcode=getelementptr
```
