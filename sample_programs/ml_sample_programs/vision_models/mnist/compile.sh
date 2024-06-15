if [ -f "$FILE" ]; then
    echo "$FILE exists. Run ./clean.sh to retrain model."
else
    printf "\n[Compile Script]: Generating TF and ONNX models\n"
    python3 mnist-cnn.py
    python3 -m tf2onnx.convert --saved-model mnist-cnn.tf --output model.onnx
fi

printf "\n[Compile Script]: Convert ONNX model to LLVM IR\n"
python3 ../../../../tools/ExtendONNXModel.py --model_path ./model.onnx  --output_model_path ./extendedmodel.onnx > expected_op_seq.txt
onnx-mlir --EmitLLVMIR extendedmodel.onnx --instrument-onnx-ops="ALL" --InstrumentBeforeAndAfterOp
mlir-translate -mlir-to-llvmir extendedmodel.onnx.mlir > model.mlir.ll

printf "\n[Compile Script]: Compile main driver program and link to TF model in LLVM IR\n"
clang -S -emit-llvm image.c -I$ONNX_MLIR_SRC/include -o main.ll
llvm-link -o model.ll -S main.ll model.mlir.ll

printf "\n[Compile Script]: Compilation complete\n"
