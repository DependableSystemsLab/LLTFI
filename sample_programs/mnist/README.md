# Sample program for running LLTFI on a TensorFlow model

This folder contains the scripts required to convert a TensorFlow model in `mnist-cnn.py` to LLVM IR.\
We link the LLVM IR of the model to an image processing program `image.c`, which invokes the model.\
This generated LLVM IR `model.ll` is instrumented, profiled, and fault injected by LLTFI.\
All of the following steps can be replicated for `mnist-nn.py`.

Dependencies (in addition to LLTFI):
---

1. [json-c](https://github.com/json-c/json-c)
   1. json-c is required for exporting intermediate ML model layer outputs in the JSON format. To install json-c, simply run the `tools/json-c-setup.sh` script. 


Initialization
---
Add the following environment variables.
```
export ONNX_MLIR_SRC=<path to onnx-mlir source>
export ONNX_MLIR_BUILD=<path to where onnx-mlir has been built>
export LLFI_BUILD_ROOT=<path to where LLFI has been built>
```

Running TensorFlow example
---
1. Train CNN on MNIST and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile.sh mnist-cnn
```

2. Select one of the test image files, e.g. `eight.png` and run LLFI on it.
```
./runllfi.sh eight.png
```


Running PyTorch example
---

1. First ensure that PyTorch framework (v1.9.0 or greater) is installed.

2. Train CNN on MNIST and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile-pytorch.sh
```

3. Select one of the test image files, e.g. `eight.png` and run LLFI on it.
```
./runllfi.sh eight.png
```


Cleaning
---
To clean all generated output files and restore to a clean source directory, run:

```
./clean.sh
```


Supplementary Information (Optional)
---

For debugging purposes, you may wish to view the generated ONNX file in this example, model.onnx, in human readable format.
You will need [onnx.proto](https://github.com/onnx/onnx/blob/master/onnx/onnx.proto) to run this command.
```
protoc --decode=onnx.ModelProto onnx.proto < model.onnx > model.onnx.readable
```

