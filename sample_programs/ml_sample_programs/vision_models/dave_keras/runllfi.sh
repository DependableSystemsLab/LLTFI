rm -rf llfi*
$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c model.ll
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe wheel.png $(cat expected_op_seq.txt)
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe wheel.png $(cat expected_op_seq.txt)
