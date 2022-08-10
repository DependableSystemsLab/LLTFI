Running TensorFlow example
---
1. Train dave_keras and compile it to LLVM IR. The final output file is `model.ll`.
```
./compile.sh
```

2. Select one of the test image files, e.g. `wheel.png` and run LLTFI on it.
```
./runllfi.sh wheel.png
```
