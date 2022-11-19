## Steps to execute Roberta-Seq-Clasification: 

1. After converting the ONNX model to LLVM IR i.e. after executig the `compile.sh` script, run fault injection experiment on the required input. Below is the command to execute the model on 'input 0'. A total of 10 different inputs (numbered 0-9) are available and can be specified in the argument. 
```
./runllfiSingleInp.sh 0
```

2. Execute the 'createPredFile' python file to generate prediction. The task here is sentiment analysis and the prediction for any input is either 'Positive' or 'Negative'. 
The output file generated is 'prediction/PredResult.txt'.
```
python createPredFile.py
```

3. Correct Reference outputs for the 10 different inputs can be found in 'prediction/RefPreds.txt'. The text inputs are present in 'inputs/input.txt'

**Reference**: [RoBERTa](https://github.com/onnx/models/tree/main/text/machine_comprehension/roberta)

