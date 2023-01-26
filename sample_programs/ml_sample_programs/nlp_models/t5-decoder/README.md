## Steps to execute t5-decoder:

1. After converting the ONNX model to LLVM IR i.e. after executing the `compile.sh` script, run fault injection experiment on the required input. Below is the command to execute the model on 'input 0'. 2 different inputs (numbered 0-1) are available and can be specified in the argument.
```
./runllfiSingleInp.sh 0
```

2. Install pytorch and the `transformers`,'onnxt5' modules for output post-processing. Execute the 'createPredFile' python file with the input number as argument to generate prediction. The task here is translation.\
Example: Input: 'translate English to French: She sings beautifully' \
Output of the model: Elle chante merveilleusement\

The output file generated is 'prediction/PredResult.txt'.
```
pip install torch
pip install transformers
pip install onnxt5

python3 createPredFile.py 0
```

3. Correct Reference outputs for the 2 different inputs can be found in 'prediction/RefPreds.txt'. The input pb files are present in `inputs` folder and the corresponding text inputs can be found in `inputs/input.txt`. `t5-decoder.ipynb` contains additional information like script for input generation, details of the encoder-decoder architecture etc.

**Reference**:
- [T5](https://github.com/onnx/models/tree/main/text/machine_comprehension/t5)
- Helper scripts from [onnxt5 repo](https://github.com/abelriboulot/onnxt5)
