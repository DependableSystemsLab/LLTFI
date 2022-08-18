printf "\n[Compile Script]: Generating TF model\n"
python3 lenet-fmnist.py

printf "\n[Compile Script]: Convert TF model to LLVM IR\n"
python3 -m tf2onnx.convert --saved-model lenet-fmnist.tf --output model.onnx
python3 ../../../../tools/ExtendONNXModel.py --model_path ./model.onnx  --output_model_path ./extendedmodel.onnx > expected_op_seq.txt
onnx-mlir --EmitLLVMIR extendedmodel.onnx --instrument-onnx-ops="ALL" --InstrumentBeforeOp --InstrumentAfterOp
mlir-translate -mlir-to-llvmir extendedmodel.onnx.mlir > model.mlir.ll

printf "\n[Compile Script]: Compile main driver program and link to TF model in LLVM IR\n"
clang -S -emit-llvm image.c -I$ONNX_MLIR_SRC/include -o main.ll
llvm-link -o model.ll -S main.ll model.mlir.ll

printf "\n[Compile Script]: Compilation complete\n"
