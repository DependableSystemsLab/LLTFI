# ML sample programs
This folder contains sample programs for fault injection for Machine Learning models. 
The 'compile.sh' script converts the ONNX model to LLVM IR. This generated LLVM IR `model.ll` is then instrumented, profiled, and fault injected by LLTFI.

## Dependencies

1. [json-c](https://github.com/json-c/json-c)
   1. json-c is required for exporting intermediate ML model layer outputs in the JSON format. To install json-c, simply run the `tools/json-c-setup.sh` script. 

## Initialization

Add the following environment variables.
```
export ONNX_MLIR_SRC=<path to onnx-mlir source>
export ONNX_MLIR_BUILD=<path to where onnx-mlir has been built>
export LLFI_BUILD_ROOT=<path to where LLFI has been built>
export LLVM_DST_ROOT=<path to where LLVM has been built>
```

## Steps to execute an application: 
1. Convert the ONNX model to LLVM IR. The final output file is `model.ll`.
```
./compile.sh
```

2. Run LLTFI on the input.
```
./runllfi.sh
```

## Executing all the ML models
Follow the below steps to build all the models and perform fault injection:
1. Modify the `input.yaml` file in the current folder if you need to add any additional options.
2. Execute the `execute_all_prog.sh` script with either the `compile` or `run` option.
	- Use the `compile` option to download and build the model.
	- Use the `run` option to perform fault injection.    
	```
	sh execute_all_prog.sh compile
	sh execute_all_prog.sh run
	```

## Supplementary Information (Optional)

For debugging purposes, you may wish to view the generated ONNX file in this example, model.onnx, in human readable format.
You will need [onnx.proto](https://github.com/onnx/onnx/blob/master/onnx/onnx.proto) to run this command.
```
protoc --decode=onnx.ModelProto onnx.proto < model.onnx > model.onnx.readable
```
