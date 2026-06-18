# Tutorial: Fault Injection for ML Models

This tutorial walks you through a complete fault injection experiment on the
`mnist` sample — a convolutional neural network that classifies handwritten
digit images.  By the end you will have:

- converted a TensorFlow model through ONNX to LLVM IR
- instrumented the IR with LLTFI's layer-aware `CustomTensorOperator` selector
- profiled the model to obtain per-layer timing data
- injected multi-bit faults into a specific layer
- interpreted the injected-fault stat files and classified outcomes

The ML pipeline has extra steps before instrumentation compared to the C/C++
tutorial (`docs/tutorial_first_experiment.md`), because LLTFI works on LLVM IR
and ML frameworks do not emit it directly.

**Prerequisites:**

| Requirement | Notes |
|---|---|
| LLTFI built | `LLFI_BUILD_ROOT` set; see `README.md` |
| LLVM 20 tools on PATH | `clang`, `opt`, `llvm-link`, `mlir-translate` |
| onnx-mlir (LLTFI branch) | `ONNX_MLIR_SRC` and `ONNX_MLIR_BUILD` set; see `README.md` |
| json-c | `tools/json-c-setup.sh` installs it |
| Python packages | `pip install tensorflow tf2onnx onnx` |

---

## 1. Set up environment variables

```bash
export LLFI_BUILD_ROOT=/path/to/LLTFI-build
export ONNX_MLIR_SRC=/path/to/onnx-mlir
export ONNX_MLIR_BUILD=/path/to/onnx-mlir/build
export PATH=/usr/lib/llvm-20/bin:$ONNX_MLIR_BUILD/Debug/bin:$PATH
```

---

## 2. Copy the sample program to a working directory

```bash
cp -r $LLFI_BUILD_ROOT/../sample_programs/ml_sample_programs/vision_models/mnist /tmp/mnist
cd /tmp/mnist
```

The directory contains:

```
mnist-cnn.py           TensorFlow model definition (CNN trained on MNIST)
mnist-cnn-pytorch.py   PyTorch alternative
image.c                C driver: loads an image, runs the model, exports layer outputs to JSON
stb_image.h            Single-header image loader used by image.c
model.onnx             Pre-trained ONNX model (regenerate with compile.sh if needed)
eight.png  five.png    Test images (one per digit class provided)
nine.png   seven.png
input.yaml             Fault injection configuration
compile.sh             Compilation pipeline (ONNX → LLVM IR)
runllfi.sh             Instrumentation, profiling, and fault injection
clean.sh               Removes all generated output files
```

---

## 3. The ML compilation pipeline

Unlike a C/C++ program, an ML model requires several steps before LLTFI can
work on it.  `compile.sh` automates all of them; this section explains each
step so you know what is produced and why.

### 3.1 Step 0 (optional): train and export the model

If `model.onnx` is already present, `compile.sh` skips this step.

```bash
python3 mnist-cnn.py                                              # train
python3 -m tf2onnx.convert --saved-model mnist-cnn.tf \
        --output model.onnx                                       # export
```

`model.onnx` is a self-contained representation of the network graph and its
trained weights.

### 3.2 Step 1: extend the ONNX model to expose intermediate layer outputs

```bash
python3 $LLFI_BUILD_ROOT/../tools/ExtendONNXModel.py \
        --model_path ./model.onnx \
        --output_model_path ./extendedmodel.onnx > expected_op_seq.txt
```

Standard ONNX models expose only the final output.  `ExtendONNXModel.py` adds
intermediate tensor outputs for every operator (conv, relu, maxpool, …) so
that `image.c` can export them to JSON during each run.  This is needed later
by `CompareLayerOutputs.py` to pinpoint which layer a fault corrupted.

`expected_op_seq.txt` receives the stdout of `ExtendONNXModel.py`: a
comma-separated list of operator position indices in execution order (e.g.
`0,1,2,3,4,5,6`).  It is passed to the model binary at runtime so the driver
knows which outputs to save.

### 3.3 Step 2: compile to MLIR with instrumentation hooks

```bash
onnx-mlir --EmitLLVMIR extendedmodel.onnx \
           --instrument-onnx-ops="ALL" \
           --InstrumentBeforeAndAfterOp
```

`onnx-mlir` lowers the ONNX graph to MLIR and then to an LLVM IR dialect,
emitting `extendedmodel.onnx.mlir`.

The two instrumentation flags are important for LLTFI:

| Flag | Effect |
|---|---|
| `--instrument-onnx-ops="ALL"` | Inserts `@OMInstrumentPoint(operator_id, flag)` calls around every operator |
| `--InstrumentBeforeAndAfterOp` | Generates both a start call (`flag=2`) and end call (`flag=1`) for each operator boundary |

`ProfilingPass` inserts calls to `lltfiMLLayer()` at these boundaries during
instrumentation.  The runtime records the dynamic instruction cycle range
`[start, end]` for each operator, enabling LLTFI to confine fault injection to
a specific layer.

### 3.4 Step 3: translate to LLVM IR

```bash
mlir-translate -mlir-to-llvmir extendedmodel.onnx.mlir > model.mlir.ll
```

### 3.5 Step 4: compile the C driver and link everything

```bash
clang -S -emit-llvm image.c -I$ONNX_MLIR_SRC/include -o main.ll
llvm-link -o model.ll -S main.ll model.mlir.ll
```

`image.c` is the program entry point: it reads the input image, calls
`run_main_graph()` (the compiled network), and writes the layer outputs to
`layeroutput.txt` in JSON format.  `llvm-link` merges the driver IR and model
IR into a single `model.ll` that LLTFI instruments as one unit.

Run all four steps at once:

```bash
./compile.sh
```

---

## 4. Examine `input.yaml`

```yaml
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=0;0;0;0;0;0;0
            - -layerName=conv;relu;matmul;maxpool;add;avgpool;softmax

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False

    tracingPropagationOption:
        maxTrace: 250
        debugTrace: False
        mlTrace: False
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 1000
        fi_type: bitflip
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        fi_max_multiple: 2
```

### 4.1 The `CustomTensorOperator` selector

`CustomTensorOperator` targets floating-point arithmetic instructions
(`fadd`, `fsub`, `fmul`, `fdiv`, `fcmp`) that fall inside the
`OMInstrumentPoint` boundary for the specified operators.  It is the
recommended selector for layer-level ML fault injection.

| `instSelMethod` | What it targets |
|---|---|
| `CustomTensorOperator` | FP arith inside named operator boundaries; needs `--instrument-onnx-ops` compilation |
| `maingraph` | All FP arith anywhere in `main_graph()`; no layer granularity |
| `insttype: include: [fadd, fmul]` | Any FP arith in the whole module; no layer or operator granularity |

### 4.2 Layer targeting with `layerNo` and `layerName`

Both options must be provided and must have the same number of semicolon-separated elements.

| `layerNo` value | Meaning |
|---|---|
| `0` | All layers of the type in the matching `layerName` position |
| `N > 0` | Only the Nth occurrence of that layer type (1-indexed) |

Examples:

```yaml
# Inject into all conv and relu layers
options:
  - -layerNo=0;0
  - -layerName=conv;relu

# Inject only into the 2nd conv layer
options:
  - -layerNo=2
  - -layerName=conv

# Inject into every layer of every type
options:
  - -layerNo=0
  - -layerName=all
```

Valid layer name values: `conv`, `relu`, `matmul`, `maxpool`, `add`,
`avgpool`, `loop`, `nonmaxs`, `unsqueeze`, `softmax`, `all`.

### 4.3 Expanding the target set with `includeInjectionTrace`

```yaml
includeInjectionTrace:
    - forward
```

`forward` expands the injection candidate set to include all instructions that
are data-flow reachable from the selected operator's output (i.e. the forward
slice).  `backward` includes all instructions that feed into the selected
operator's inputs (the backward slice).  Omit this key to inject only into the
instructions directly inside the operator boundary.

### 4.4 Multi-fault injection

The MNIST `input.yaml` injects up to 2 faults per run, separated by a random
number of dynamic instructions drawn from `[1, 500]`:

```yaml
fi_max_multiple: 2
window_len_multiple_startindex: 1
window_len_multiple_endindex: 500
```

| Key | Meaning |
|---|---|
| `fi_max_multiple` | Maximum number of faults per run (≤ 100) |
| `window_len_multiple_startindex` | Lower bound on the inter-fault instruction gap |
| `window_len_multiple_endindex` | Upper bound on the inter-fault instruction gap |

`fi_max_multiple` and `window_len` are mutually exclusive — use only one.

For the full key reference see `docs/input_yaml_guide.md` and
`docs/input_masterlist_ml.yaml`.

---

## 5. Instrument

```bash
$LLFI_BUILD_ROOT/bin/instrument --readable \
    -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lprotobuf \
    model.ll
```

The `-L` and `-l` flags link the onnx-mlir runtime libraries and json-c into
the instrumented binaries.

`instrument` produces the `llfi/` directory:

```
llfi/
  model-profiling.exe          Binary for the profiling pass
  model-faultinjection.exe     Binary for fault injection
  model-profiling.ll           Instrumented IR (profiling version)
  model-faultinjection.ll      Instrumented IR (fault injection version)
  model-llfi_index.ll          IR annotated with LLFI index numbers
```

Two top-level files are also written:

| File | Contents |
|---|---|
| `llfi.log.compilation.txt` | Full pass output; check here if instrumentation fails |
| `llfi.config.compiletime.txt` | Summary of what was selected (failure class, layer, targets) |

Check `llfi.config.compiletime.txt`:

```
failure_class=HardwareFault
failure_mode=CustomTensorOperator
targets=<FP arith inside conv;relu;... operator boundaries>
injector=<fi_type>
```

If the file shows `0 candidate instructions`, either the operator names do not
match those in the model or the model was compiled without
`--instrument-onnx-ops`.

---

## 6. Profile

```bash
$LLFI_BUILD_ROOT/bin/profile \
    ./llfi/model-profiling.exe \
    eight.png \
    $(cat expected_op_seq.txt)
```

The arguments after the executable are passed to the model:
- `eight.png` — the input image
- `$(cat expected_op_seq.txt)` — the layer output sequence, so the driver knows
  which intermediate tensors to save to `layeroutput.txt`

Profiling runs the model once without injecting faults.  It produces:

| File | Contents |
|---|---|
| `llfi.stat.prof.txt` | Total cycle count, plus one `ml_layer` line per operator |
| `llfi.stat.totalindex.txt` | Number of unique injectable instruction indices |
| `llfi/baseline/golden_std_output` | Stdout of the fault-free run (predicted digit) |
| `layeroutput.txt` | Per-layer tensor values from the golden run (JSON format) |

The ML-specific entries in `llfi.stat.prof.txt` look like:

```
total_cycle=4827613
ml_layer=0,conv,12345,89012
ml_layer=1,relu,89013,105400
ml_layer=2,conv,105401,312800
...
```

Each `ml_layer` line records: sequential layer number, operator type, start
dynamic-instruction cycle, end dynamic-instruction cycle.  `injectfault` uses
this data to confine injection to cycles that fall within the requested layer.

Move `layeroutput.txt` to a safe location before running fault injection, since
each faulty run will overwrite it:

```bash
cp layeroutput.txt llfi/baseline/golden_layeroutput.txt
```

---

## 7. Inject faults

```bash
$LLFI_BUILD_ROOT/bin/injectfault \
    ./llfi/model-faultinjection.exe \
    eight.png \
    $(cat expected_op_seq.txt)
```

This runs the model `numOfRuns` times (1000 in the sample `input.yaml`).
Each run draws a random cycle from within the layer timing ranges recorded
during profiling, injects up to `fi_max_multiple` bit-flips separated by a
random gap, and records the outcome.

On completion, `llfi/` contains:

```
llfi/
  baseline/
    golden_std_output             Reference output from profiling
    golden_layeroutput.txt        (you copied this manually above)
  std_output/
    std_outputfile-run-0-0        Stdout from run 0
    std_outputfile-run-0-1        Stdout from run 1
    ...
  error_output/
    errorfile-run-0-N             Written only for crashed or timed-out runs
  llfi_stat_output/
    llfi.stat.fi.injectedfaults.0-0.txt   Injection details for run 0
    llfi.stat.fi.injectedfaults.0-1.txt
    ...
  prog_output/
    layeroutput.txt               Layer output from the most recent run
                                  (overwritten each run — save per-run if needed)
```

---

## 8. Interpret the results

### 8.1 Injected fault stat files

Each `llfi.stat.fi.injectedfaults.<exp>-<run>.txt` records what happened in
one trial.  For a multi-fault run it contains one line per injected fault:

```
FI stat: fi_type=bitflip, fi_max_multiple=2, fi_index=1042, fi_cycle=156203,
         fi_reg_index=0, fi_reg_pos=0, fi_reg_width=32, fi_bit=17, opcode=fmul
ml_layer_name=conv
ml_layer_number=2
FI stat: fi_type=bitflip, fi_max_multiple=2, fi_index=874, fi_cycle=156387,
         fi_reg_index=0, fi_reg_pos=0, fi_reg_width=32, fi_bit=4, opcode=fadd
ml_layer_name=conv
ml_layer_number=2
```

The fields from the C/C++ tutorial apply here too.  The additional ML fields:

| Field | Meaning |
|---|---|
| `ml_layer_name` | Operator type of the layer where the fault landed (`conv`, `relu`, …) |
| `ml_layer_number` | Sequential layer index (matches the `ml_layer=N,…` line in `llfi.stat.prof.txt`) |

### 8.2 Classifying outcomes

The outcome classification is the same as for C/C++ programs:

| Outcome | How to identify |
|---|---|
| **Masked** | `std_outputfile` matches `golden_std_output` (same predicted digit); no `errorfile` |
| **SDC** | `std_outputfile` differs (wrong digit predicted); no `errorfile` |
| **Crash** | `errorfile` present with signal number (e.g. `-11` = SIGSEGV) |
| **Hang** | `errorfile` present with timeout message |

Quick batch comparison:

```bash
for f in llfi/std_output/std_outputfile-run-0-*; do
    echo -n "$f: "
    if diff -q "$f" llfi/baseline/golden_std_output > /dev/null 2>&1; then
        echo "MASKED"
    elif [ -f "llfi/error_output/errorfile-run-0-$(basename $f | sed 's/.*-//')" ]; then
        echo "CRASH/HANG"
    else
        echo "SDC"
    fi
done
```

For MNIST with 1000 runs you should expect a high masked rate (a single
bit-flip in a floating-point multiply is usually too small to change the
final argmax) with occasional SDC (wrong digit) and rare crashes.

### 8.3 Layer-level analysis with `CompareLayerOutputs.py`

To find which layer first produced a corrupted output, compare the JSON layer
outputs from a faulty run to the golden run.  This requires saving
`layeroutput.txt` from each injection run before the next one overwrites it;
the simplest approach is to add a post-run copy step or reduce `numOfRuns` to 1
when doing targeted investigation.

```bash
# Run a single fault injection with the stat file pinning the fault
$LLFI_BUILD_ROOT/bin/injectfault \
    ./llfi/model-faultinjection.exe \
    eight.png \
    $(cat expected_op_seq.txt)

# Compare layer outputs
python3 $LLFI_BUILD_ROOT/../tools/CompareLayerOutputs.py \
    --golden llfi/baseline/golden_layeroutput.txt \
    --faulty layeroutput.txt
```

`CompareLayerOutputs.py` prints the first layer whose output tensor differs
from the golden run, along with a summary of how many elements changed and by
how much.  With `pygraphviz` installed it also writes a dot graph highlighting
the affected layers.

---

## 9. Targeting a specific layer

To restrict injection to a single layer, edit `input.yaml`.  For example, to
inject only into the first convolutional layer:

```yaml
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=1
            - -layerName=conv

    regSelMethod: regloc
    regloc: dstreg

runOption:
    - run:
        numOfRuns: 200
        fi_type: bitflip
```

Then delete the `llfi/` directory and re-run from instrumentation:

```bash
rm -rf llfi llfi.stat.prof.txt llfi.stat.totalindex.txt \
       llfi.config.compiletime.txt llfi.log.compilation.txt
$LLFI_BUILD_ROOT/bin/instrument --readable \
    -L $ONNX_MLIR_BUILD/Debug/lib -lcruntime -ljson-c -lprotobuf \
    model.ll
$LLFI_BUILD_ROOT/bin/profile \
    ./llfi/model-profiling.exe eight.png $(cat expected_op_seq.txt)
$LLFI_BUILD_ROOT/bin/injectfault \
    ./llfi/model-faultinjection.exe eight.png $(cat expected_op_seq.txt)
```

Alternatively, `runllfi.sh` wraps these three steps (it deletes `llfi*/` first):

```bash
./runllfi.sh
```

---

## 10. PyTorch path

If you prefer PyTorch, use `compile-pytorch.sh` instead of `compile.sh`:

```bash
./compile-pytorch.sh
```

The PyTorch path compiles without `--instrument-onnx-ops`, so the IR contains
no `OMInstrumentPoint` calls.  This means:

- `CustomTensorOperator` **cannot** be used — use `maingraph` or `insttype` instead
- No per-layer timing data in `llfi.stat.prof.txt` — faults are distributed
  uniformly across all targeted instructions rather than being confined to a layer
- `CompareLayerOutputs.py` is not applicable because `expected_op_seq.txt` is
  not generated and `layeroutput.txt` is not written

Example `input.yaml` for PyTorch:

```yaml
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - maingraph

    regSelMethod: regloc
    regloc: dstreg

runOption:
    - run:
        numOfRuns: 200
        fi_type: bitflip
```

---

## 11. Next steps

- **Vary the layer**: change `layerName` and `layerNo` to compare fault
  sensitivity across layers.
- **Vary the fault model**: change `fi_type` to `stuck_at_0` or `stuck_at_1`
  to model permanent hardware faults instead of transient bit-flips.
- **Try a larger model**: `sample_programs/ml_sample_programs/vision_models/`
  contains ResNet-50, VGG-16, GoogLeNet, and others — all use the same workflow.
- **Instruction duplication**: apply `SEDPasses.so` before instrumentation to
  evaluate soft-error detection coverage.  See
  `llvm_passes/instruction_duplication/README.md`.
- **Batch across multiple models**: see `bin/batchInstrument.py`,
  `batchProfile.py`, and `batchInjectfault.py` for running an experiment
  campaign across many programs or fault modes in one call.
- **Read the architecture**: `architecture.md` §2.5 explains how
  `CustomTensorOperatorInstSelector` and `OMInstrumentPoint` work together.
