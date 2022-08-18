# Sample program for running LLTFI on a TensorFlow model

This folder contains the scripts required to convert a TensorFlow model in `mnist-cnn.py` to LLVM IR.\
We link the LLVM IR of the model to an image processing program `image.c`, which invokes the model.\
This generated LLVM IR `model.ll` is instrumented, profiled, and fault injected by LLTFI.\
All of the following steps can be replicated for `mnist-nn.py`.

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

