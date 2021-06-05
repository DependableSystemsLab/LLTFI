printf "\n[Compile Script]: Generating TF model\n"
python3 $1.py

printf "\n[Compile Script]: Convert TF model to LLVM IR\n"
python3 -m tf2onnx.convert --saved-model $1.tf --output model.onnx
onnx-mlir --EmitLLVMIR model.onnx
mlir-translate -mlir-to-llvmir model.onnx.mlir > model.mlir.ll

printf "\n[Compile Script]: Compile main driver program and link to TF model in LLVM IR\n"
clang -S -emit-llvm image.c -I$ONNX_MLIR_SRC/include -o main.ll
llvm-link -o model.ll -S main.ll model.mlir.ll

printf "\n[Compile Script]: Compilation complete\n"

