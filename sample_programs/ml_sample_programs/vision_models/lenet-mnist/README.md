Running TensorFlow example
---
1. Train lenet-mnist and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile.sh lenet-mnist
```

2. Select one of the test image files, e.g. `eight.png` and run LLTFI on it.
```
./runllfi.sh eight.png
```
