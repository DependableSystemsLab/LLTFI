rm -rf llfi*
inputSample=$1

$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe "inputs/inp$inputSample/input_0.pb" "inputs/inp$inputSample/input_1.pb" 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe "inputs/inp$inputSample/input_0.pb" "inputs/inp$inputSample/input_1.pb" 0
