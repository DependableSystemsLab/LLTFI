printf "[Compile Script]: Getting the ONNX model\n"

FILE=t5-encoder-12.onnx
if [ -f "$FILE" ]; then
    echo "$FILE exists."
else
    echo "$FILE does not exist."
    wget https://github.com/onnx/models/raw/main/validated/text/machine_comprehension/t5/model/t5-encoder-12.onnx
fi

DEC_FILE=t5-decoder-with-lm-head-12.onnx
if [ -f "$DEC_FILE" ]; then
    echo "$DEC_FILE exists."
else
    echo "$DEC_FILE does not exist."
    wget https://github.com/onnx/models/raw/main/validated/text/machine_comprehension/t5/model/t5-decoder-with-lm-head-12.onnx
fi

printf "\n[Compile Script]: Convert TF model to LLVM IR\n"
onnx-mlir --EmitLLVMIR  --instrument-onnx-ops="ALL" --InstrumentBeforeAndAfterOp t5-encoder-12.onnx
mlir-translate -mlir-to-llvmir t5-encoder-12.onnx.mlir > model.mlir.ll

printf "\n[Compile Script]: Compile main driver program and link to TF model in LLVM IR\n"
clang++ -DONNX_ML=1 input.c -o main.ll -O0 -S -emit-llvm -lonnx_proto -lprotobuf -I$ONNX_MLIR_SRC/include
llvm-link -o model.ll -S main.ll model.mlir.ll

printf "\n[Compile Script]: Generate model.exe \n"
$LLVM_DST_ROOT/bin/llc -filetype=obj -o model.o model.ll -O0 --relocation-model=pic
clang++ -o model.exe model.o -L$LLFI_BUILD_ROOT/bin/../runtime_lib -lllfi-rt -lpthread -L /Debug/lib -Wl,-rpath $LLFI_BUILD_ROOT/bin/../runtime_lib -I$ONNX_MLIR_SRC/include -O0 -lonnx_proto -lprotobuf -lcruntime -ljson-c

printf "\n[Compile Script]: Compilation complete\n"
