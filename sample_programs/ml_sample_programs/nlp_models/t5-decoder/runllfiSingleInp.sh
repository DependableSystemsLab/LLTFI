rm -rf llfi*
inputSample=$1

cp -r "inputs/inp$inputSample/input_0.pb" .
cp -r "inputs/inp$inputSample/input_1.pb" .

$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe input_0.pb input_1.pb 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe input_0.pb input_1.pb 0
