Running TensorFlow example
---
1. Train lenet-fmnist and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile.sh lenet-fmnist
```

2. Select one of the test image files, e.g. `nine.png` and run LLTFI on it.
```
./runllfi.sh nine.png
```
