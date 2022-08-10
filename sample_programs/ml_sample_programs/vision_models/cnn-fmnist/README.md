Fault injection for cnn-fmnist
---
1. Compile the onnx model `model.onnx` to LLVM IR. The final output file is `model.ll`.
```
./compile.sh
```

2. Select one of the test image files, e.g. `wheel.png` and run LLTFI on it.
```
./runllfi.sh nine.png
```
