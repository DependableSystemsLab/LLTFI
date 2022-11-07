## Steps to generate text output: 
1. After converting the ONNX model to LLVM IR, run LLTFI on the required input. Below is the command to execute the model on 'input 0'. A total of 10 different inputs are available numbered 0-9.
```
./runllfiSingleInp.sh 0
```

2. Execute the 'createJsonOut' script with the 'input number' as argument to create text output. The output JSON files can be found in the newly generated 'out' folder.
```
./createJsonOut.sh 0
```

`bert_helper` contains files from BERT(https://github.com/google-research/bert) repository.
