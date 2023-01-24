rm -rf llfi*
inputSample=$1

cp -r "input/inp$inputSample/"* .

$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe "input0_$inputSample.pb" "input1_$inputSample.pb" "input2_$inputSample.pb" "input3_$inputSample.pb" 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe "input0_$inputSample.pb" "input1_$inputSample.pb" "input2_$inputSample.pb" "input3_$inputSample.pb" 0
