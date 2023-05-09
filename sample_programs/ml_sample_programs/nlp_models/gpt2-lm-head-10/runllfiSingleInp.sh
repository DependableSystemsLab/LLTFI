#!/bin/bash

rm -rf llfi*
inputSample=$1

#cp -r "./inputs/input_$inputSample.pb" .

$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll --use-ml-specific-rt --enable-ML-FI-stats
$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe "input_$inputSample.pb" 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe "input_$inputSample.pb" 0
