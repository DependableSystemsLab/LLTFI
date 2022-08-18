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

## Supplementary Information (Optional)

For debugging purposes, you may wish to view the generated ONNX file in this example, model.onnx, in human readable format.
You will need [onnx.proto](https://github.com/onnx/onnx/blob/master/onnx/onnx.proto) to run this command.
```
protoc --decode=onnx.ModelProto onnx.proto < model.onnx > model.onnx.readable
```
