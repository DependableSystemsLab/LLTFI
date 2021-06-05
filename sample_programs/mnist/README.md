# Sample program for running LLFI on a TensorFlow model

This folder contains the scripts required to convert a TensorFlow model in `mnist-cnn.py` to LLVM IR.\
We link the LLVM IR of the model to an image processing program `image.c`, which invokes the model.\
This generated LLVM IR `model.ll` is instrumented, profiled, and fault injected by LLFI.\
All of the following steps can be replicated for `mnist-nn.py`.

Dependencies (in addition to LLFI):
---
1. 64 Bit Machine (preferably with GPU for faster training)
2. TensorFlow framework (v2.0 or greater)
3. numpy package (part of TensorFlow)
4. [tensorflow-onnx](https://github.com/onnx/tensorflow-onnx)
   - Installation with pip is sufficient
5. [onnx-mlir](https://github.com/onnx/onnx-mlir)
   - Must use the designated LLVM commit to work
   - Ensure that the version of libprotoc is compatible

Running
---
1. Add the following environment variables.
```
export ONNX_MLIR_SRC=<path to onnx-mlir source>
export ONNX_MLIR_BUILD=<path to where onnx-mlir has been built>
export LLFI_BUILD_ROOT=<path to where LLFI has been built>
```

2. Train CNN on MNIST and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile.sh mnist-cnn
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

