#!/bin/bash

rm -rf llfi*
$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lonnx_proto -lprotobuf model.ll --use-ml-specific-rt --enable-ML-FI-stats

echo "Compiling executables..."

for i in {0..9}
do

$LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe "input_${i}.pb" 0
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe "input_${i}.pb" 0
python3 createPredFile.py $i

cd results
mkdir $i
cd $i
cp -R ../../llfi/llfi_stat_output/ .
#cp -R ../../llfi/prog_output/ .
cp -R ../../out .
cd ../..
done
