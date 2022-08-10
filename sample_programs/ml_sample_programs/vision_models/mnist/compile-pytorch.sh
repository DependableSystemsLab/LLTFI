printf "\n[Compile Script]: Generating PyTorch model\n"
python3 mnist-cnn-pytorch.py --epochs 5 --save-onnx

printf "\n[Compile Script]: Convert model to LLVM IR\n"
onnx-mlir --EmitLLVMIR model.onnx
mlir-translate -mlir-to-llvmir model.onnx.mlir > model.mlir.ll

printf "\n[Compile Script]: Compile main driver program and link to TF model in LLVM IR\n"
clang -S -emit-llvm image.c -I$ONNX_MLIR_SRC/include -o main.ll
llvm-link -o model.ll -S main.ll model.mlir.ll

printf "\n[Compile Script]: Compilation complete\n"

