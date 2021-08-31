# Sample program for running LLTFI on a TensorFlow model

This folder contains the scripts required to convert a TensorFlow model in `mnist-cnn.py` to LLVM IR.\
We link the LLVM IR of the model to an image processing program `image.c`, which invokes the model.\
This generated LLVM IR `model.ll` is instrumented, profiled, and fault injected by LLTFI.\
All of the following steps can be replicated for `mnist-nn.py`.

Dependencies (in addition to LLFI):
---
1. 64 Bit Machine (preferably with GPU for faster training)
2. TensorFlow framework (v2.0 or greater)
3. numpy package (part of TensorFlow)
4. [tensorflow-onnx](https://github.com/onnx/tensorflow-onnx)
   - Installation with pip is sufficient
   ```
   pip install tf2onnx
   ```
5. [onnx-mlir](https://github.com/onnx/onnx-mlir)
   1. Must use the designated compatible LLVM commit to work
   
    Because onnx-mlir requires a specific LLVM commit, and LLVM 12.0 takes a long time to completely build,
    the following is a short cut to checking out the LLVM commit, and building only the necessary LLVM targets.
    Also, please download and select Ninja as the build tool.

    ```
    git clone https://github.com/llvm/llvm-project.git
    # Check out a specific branch that is known to work with ONNX MLIR.
    cd llvm-project && git checkout ebe408ad8003c946ef871b955ab18e64e82697cb && cd ..
    ```
    ```
    mkdir llvm-project/build
    cd llvm-project/build
    
    cmake -G Ninja ../llvm \
      -DLLVM_ENABLE_PROJECTS="clang;mlir;tools" \
      -DLLVM_BUILD_TESTS=ON \
      -DLLVM_TARGETS_TO_BUILD="host" \
      -DLLVM_ENABLE_ASSERTIONS=ON \
      -DLLVM_ENABLE_RTTI=ON
    
    cmake --build . --target clang check-mlir mlir-translate opt llc lli llvm-dis llvm-link
    ```

    2. Ensure that the version of libprotoc is compatible.
    ```
    curl -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.17.0/protobuf-all-3.17.0.zip
    unzip protobuf-all-3.17.0.zip
    cd protobuf-3.17.0
    
    ./configure
    make
    make check
    sudo make install
    sudo ldconfig # refresh shared library cache.
    ```

    3. Finally, you may follow the rest of the steps in [onnx-mlir](https://github.com/onnx/onnx-mlir). 


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

Control Flow Graph Visualization Example
---
In this example, we run the TensorFlow example and generate a visual image of a CFG after fault injection.

1. Checkout the ml_graph branch.
```
git checkout origin/ml_graph
```

2. Run steps 1-2 of the [TensorFlow example](#running-tensorflow-example) above.

3. Generate the diff report and trace dot file. In this example, we generate a diff report on the first fault injection run.
```
./generategraph.sh llfi/llfi_stat_output/llfi.stat.trace.0-0.txt
```

4. Either (i) visualize the dot file using xdot or (ii) generate a PDF from the trace dot file.
```
xdot diffgraph.dot
```
```
dot -Tpdf diffgraph.dot -o diffgraph.pdf
```

Supplementary Information (Optional)
---

For debugging purposes, you may wish to view the generated ONNX file in this example, model.onnx, in human readable format.
You will need [onnx.proto](https://github.com/onnx/onnx/blob/master/onnx/onnx.proto) to run this command.
```
protoc --decode=onnx.ModelProto onnx.proto < model.onnx > model.onnx.readable
```

