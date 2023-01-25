## Steps to execute gpt2-lm-head:

1. After converting the ONNX model to LLVM IR i.e. after executing the `compile.sh` script, run fault injection experiment on the required input. Below is the command to execute the model on 'input 0'. A total of 5 different inputs (numbered 0-4) are available and can be specified in the argument.
```
./runllfiSingleInp.sh 0
```

2. Install the `transformers` module for output post-processing. Execute the 'createPredFile' python file with the input number as argument to generate prediction. The task here is text prediction.\
Example: Input: The chair is white and the table is \
Output of the model: black\

The output file generated is 'prediction/PredResult.txt'.
```
pip install transformers

python3 createPredFile.py 0
```

3. Correct Reference outputs for the 5 different inputs can be found in 'prediction/RefPreds.txt'. The input pb files are present in `inputs` folder and the corresponding text inputs can be found in `inputs/input.txt`. `gpt2.ipynb` contains additional information like script for input generation, details of gpt2-lm-head-10 etc.

**Reference**: [GPT2](https://github.com/onnx/models/tree/main/text/machine_comprehension/gpt-2)
