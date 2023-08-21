
# ISSRE 2023 Artifacts Evaluation Guide

This README file describes the structure of this project, our fault injection (FI) tool, benchmarks, instructions to build the tool, and, finally, instructions to reproduce the experiments reported in our paper.


## Artifacts Description
### LLTFI - Our FI tool

LLTFI (Low Level Tensor Fault Injector) is a unified SWiFI (Software-implemented fault injection) tool that supports fault injection of both C/C++ programs and ML applications written using high-level frameworks such as TensorFlow and PyTorch. LLTFI supports FI in both vision-based and NLP-based models.

In this work, we modified LLTFI to carry out FI in Large Language Models (LLMs).

#### More Information on LLTFI (Optional - Not Required for AE)
[LLTFI Documentation](https://github.com/DependableSystemsLab/LLTFI)\
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
    ├── config/                # CMake Config
    ├── docker/                # Constains Instructions and Docker file for obtaining LLTFI via docker.
    ├── docs/                  # Documentation of different FI configurations that can be used with LLTFI
    ├── images/                # Images used in README files
    ├── installer/
        ├── InstallLLTFI.py    # Installation script to install LLTFI and its dependencies.
    ├── llvm_passes/           # LLVM Passes used by LLTFI's core for profiling and FI.
    ├── runtime_lb/            # LLTFI's runtime components
    ├── sample_programs/       # Large, real-world, C++ End-to-End tests and ML models.
        ├── cpp_sample_programs/ 
        ├── ml_sample_programs/
            ├── nlp_models/    # Contains NLP models.
            ├── vision_models/ # Contains Vision-based ML models.
    ├── test_suite/            # Small C++ End-to-End tests for validating LLTFI's functionality.
    ├── tools/                 # Helper scripts and 3-rd part tools used by LLTFI.
    ├── web-app/               # A GUI frontend of LLTFI (now depreciated)
    ├── LICENSE.txt
    ├── CREDITS.txt
    ├── README.md              # README file for ISSRE'23 AE 
    └── README_LLTFI.md        # README file for instralling and testing LLTFI.
    ...

