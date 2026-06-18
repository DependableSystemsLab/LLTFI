# Sample program for running LLTFI on a TensorFlow model

This folder contains the scripts required to convert a TensorFlow model in `mnist-cnn.py` to LLVM IR.\
We link the LLVM IR of the model to an image processing program `image.c`, which invokes the model.\
This generated LLVM IR `model.ll` is instrumented, profiled, and fault injected by LLTFI.\
All of the following steps can be replicated for `mnist-nn.py`.


Pre-set Input YAML Configurations
---

Replace `input.yaml` with `input1.yaml` (for reduced number of runs) and `input2.yaml` for greater fault intensity.



Running a  Pre-trained Model on MNIST example
---

1. Compile a pre-trained CNN model on MNIST to LLVM IR. The final output file is `model.ll`.
```
./compile.sh
```

2. Run LLTFI on one of the images, `eight.png` by default.
```
./runllfi.sh
```

3. Compute the number of SDCs where the model output deviates from the golden output.
```
./check_sdc.sh
```

4. Compute the number of critical SDCs where the model output causes a different prediction from the golden prediction.
```
python check_critical_sdc.py
```


Cleaning
---
To clean all generated output files and restore to a clean source directory, run:

```
./clean.sh
```

