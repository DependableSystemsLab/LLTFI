# Selective Instrution Duplication

Selective Instruction Duplication (SID) is a technique for selectively duplicating certain instructions in the program to detect soft errors. LLTFI has been extended to perform SID at the LLVM IR level, and correct the errors. Please refer to this [paper](https://www.dropbox.com/s/lgr3ed75sy0fq2p/issre22-camera-ready.pdf?dl=0) for more details about SID.

The Selective Instruction Duplication pass is done in `LLTFI/llvm_passes/instruction_duplication/InstructionDuplication.cpp`.

The correction logic can be found in : `LLTFI/llvm_passes/instruction_duplication/shared_lib/SIDHelperFunctions.cpp`

## Steps to perform SID :

1. To generate the libraries required for the correction logic, execute the following scripts:
```
cd LLTFI/llvm_passes/instruction_duplication/shared_lib/
sh build.sh 
sh compile_shrd_lib.sh

```

2. `model.ll` is the generated LLVM IR file in the sample applications. Add the following commands in the `compile.sh` files after `model.ll` is generated. (Add it after this command: `llvm-link -o model.ll -S main.ll model.mlir.ll`).

```
# Perform instruction duplication
$LLVM_BUILD_PATH/bin/opt -load ../../../../build/llvm_passes/instruction_duplication/SEDPasses.so \
  --InstructionDuplicationPass -operatorName=all \
  --enableChainDuplication --enable-new-pm=0 -S model.ll -o model_change.ll \
  > /dev/null

# Link the comparision checks
llvm-link -o model_change.ll -S model_change.ll SIDHelperFunctions.ll

# Inline the comparison checks
$LLVM_BUILD_PATH/bin/opt model_change.ll -always-inline -S -o model.ll

```

- Here `LLVM_BUILD_PATH` is the directory where LLVM is built
- Use `--enableChainDuplication` to toggle between ACD (Arithmetic Chain Duplication) and AID (Arithmetic SID). Default value if nothing is specified: False 
- Use `--llfiIndex` to specify the LLFI index (unique instruction number) to do SID. Default value if nothing is specified:all

3. Copy the `SIDHelperFunctions.ll` from `LLTFI/llvm_passes/instruction_duplication/shared_lib/` folder to the sample application folder and execute the application following the steps mentioned in their README.
