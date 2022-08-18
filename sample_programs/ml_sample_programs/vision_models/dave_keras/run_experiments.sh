#!/bin/bash


python3 ../../tools/create_input_yaml.py --random_layer --runs=25 --fault=1 --model_path=model.onnx
$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c model.ll > /dev/null

for j in $(seq 1 1 50)
do
    echo "Selecting a random layer for FI: ${j}"
    rm input.yaml

    python3 ../../tools/create_input_yaml.py --random_layer --runs=25 --fault=1 --model_path=model.onnx
    #$LLFI_BUILD_ROOT/bin/instrument --readable -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c model.ll > /dev/null
    $LLFI_BUILD_ROOT/bin/profile ./llfi/model-profiling.exe wheel.png $(cat expected_op_seq.txt)
    $LLFI_BUILD_ROOT/bin/injectfault ./llfi/model-faultinjection.exe wheel.png $(cat expected_op_seq.txt)

    #python3 ../../tools/CompareLayerOutputs.py ./llfi/baseline/layeroutput.prof.txt ./llfi/prog_output/ model.onnx -f --summary --fiStatsDir ./llfi/llfi_stat_output/ --getFIStatsRQ2 > "./Result_audit/${j}.txt"
    echo $(cat ./llfi/std_output/* | grep prediction | awk -F' ' '{print $6}' | grep -v "0.038718" | wc -l | awk '{if (NR != 0) {n += $0}} END{printf "%.03f\n", n/25}') > "Result/${j}.txt"
    #rm -r llfi/std_output/*
done
