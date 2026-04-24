# Tutorial: Your First Fault Injection Experiment

This tutorial walks you through a complete fault injection experiment using the
`factorial` sample program.  By the end you will have:

- compiled a C program to LLVM IR
- instrumented it with LLTFI
- run the profiling pass
- run fault injection
- interpreted every output file

**Prerequisites:** LLTFI is installed and `LLFI_BUILD_ROOT` points at the build
directory (see `README.md` for setup).

---

## 1. Set up environment variables

```bash
export LLFI_BUILD_ROOT=/path/to/LLTFI-build
export PATH=/usr/lib/llvm-20/bin:$PATH   # so clang, opt, llvm-dis are found
```

---

## 2. Copy the sample program to a working directory

```bash
cp -r $LLFI_BUILD_ROOT/../sample_programs/cpp_sample_programs/factorial /tmp/factorial
cd /tmp/factorial
```

The directory contains:

```
factorial.c       Source file
compileAndRun.sh  Convenience script (wraps the three steps below)
input.yaml        Fault injection configuration
```

---

## 3. Compile to LLVM IR

LLTFI works on LLVM bitcode (`.bc`) or human-readable IR (`.ll`).  Compile
with `-emit-llvm`:

```bash
clang -emit-llvm -g -S factorial.c -o factorial.ll
```

The `-g` flag adds debug information, which improves LLFI's index maps.

---

## 4. Examine `input.yaml`

```yaml
compileOption:
    instSelMethod:
      - insttype:
          include:
            - all
          exclude:
            - ret

    regSelMethod: regloc
    regloc: allreg

    tracingPropagation: False

    tracingPropagationOption:
        maxTrace: 250
        debugTrace: False
        generateCDFG: True

runOption:
    - run:
        numOfRuns: 5
        fi_type: bitflip
```

This says:

- **Target**: every instruction type except `ret` (returning mid-function would
  corrupt the stack, giving meaningless results)
- **Register**: all registers of each targeted instruction (`allreg`)
- **Tracing**: off (keeps output small for this tutorial)
- **Experiment**: 5 fault injection runs, each flipping a randomly chosen bit
  in a randomly chosen targeted register

For the full key reference, see `docs/input_yaml_guide.md`.

---

## 5. Instrument

```bash
$LLFI_BUILD_ROOT/bin/instrument --readable factorial.ll
```

This runs the LLVM pass pipeline on `factorial.ll` and produces the `llfi/`
directory:

```
llfi/
  factorial-profiling.exe     Binary for the profiling pass
  factorial-faultinjection.exe  Binary for fault injection
  factorial-profiling.ll      Instrumented IR (profiling version)
  factorial-faultinjection.ll Instrumented IR (fault injection version)
  factorial-llfi_index.ll     IR annotated with LLFI index numbers
```

Two top-level files are also written:

| File | Contents |
|------|----------|
| `llfi.log.compilation.txt` | Full output of the instrumentation pass (errors appear here) |
| `llfi.config.compiletime.txt` | Summary of what the pass selected (failure class, mode, targets) |

Check `llfi.config.compiletime.txt` to verify the selector matched what you
expected:

```
failure_class=HardwareFault
failure_mode=SpecifiedInstructionTypes
targets=<include list in yaml>
injector=<fi_type>
```

---

## 6. Profile

```bash
$LLFI_BUILD_ROOT/bin/profile ./llfi/factorial-profiling.exe 6
```

The argument `6` is the program input (compute 6! = 720).

Profiling runs the program once without injecting any faults.  It produces:

| File | Contents |
|------|----------|
| `llfi.stat.prof.txt` | Total dynamic instruction count: `total_cycle=<N>` |
| `llfi.stat.totalindex.txt` | Number of unique injectable instruction indices: `totalindex=<N>` |
| `llfi/baseline/golden_std_output` | Stdout of the fault-free run — the reference for SDC detection |
| `llfi/baseline/mcf.prof.out` | Program-specific profiling output (if any) |

For factorial with input 6, `totalindex` will be around 50–100 (a small
program) and `golden_std_output` will contain `720`.

---

## 7. Inject faults

```bash
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/factorial-faultinjection.exe 6
```

This runs the program `numOfRuns` times (5 in our `input.yaml`), injecting one
fault per run.  Each run:

1. Reads `llfi.config.runtime.txt` to decide where and what to inject.
2. Selects a random injectable instruction and a random bit.
3. Flips that bit at the moment the instruction executes.
4. Records the outcome.

On completion, the `llfi/` directory is populated:

```
llfi/
  baseline/
    golden_std_output          Reference output (from profiling)
  std_output/
    std_outputfile-run-0-0     Stdout of run 0 (first experiment, trial 0)
    std_outputfile-run-0-1     Stdout of run 1
    ...
  error_output/
    errorfile-run-0-N          Written only for runs that crashed or hung
  llfi_stat_output/
    llfi.stat.fi.injectedfaults.0-0.txt   Injection details for run 0
    llfi.stat.fi.injectedfaults.0-1.txt   Injection details for run 1
    ...
  prog_output/
    (disk output from the program, if any)
```

---

## 8. Interpret the results

### 8.1 Stat files — what was injected

Each `llfi.stat.fi.injectedfaults.<exp>-<run>.txt` records exactly what happened
during one trial.  Example:

```
FI stat: fi_type=bitflip, fi_max_multiple=-1, fi_index=12, fi_cycle=47,
         fi_reg_index=0, fi_reg_pos=0, fi_reg_width=64, fi_bit=28, opcode=load
```

| Field | Meaning |
|-------|---------|
| `fi_type` | Fault type injected (`bitflip`, `stuck_at_0`, `stuck_at_1`) |
| `fi_index` | LLFI index of the targeted instruction (matches `factorial-llfi_index.ll`) |
| `fi_cycle` | Dynamic instruction count at the moment of injection |
| `fi_reg_index` | Which register of the instruction was targeted (0 = destination) |
| `fi_reg_pos` | Word position within a multi-word register (usually 0) |
| `fi_reg_width` | Register width in bits (32 or 64) |
| `fi_bit` | Which bit was flipped (0 = LSB) |
| `opcode` | LLVM IR opcode of the targeted instruction |
| `fi_max_multiple` | Number of faults injected; -1 = single fault |

These values are sufficient to reproduce a specific run — copy them back into
`input.yaml` as pin fields (see `docs/input_yaml_guide.md` §Pinning a fault).

### 8.2 Classifying outcomes

Compare each run's `std_outputfile-run-<exp>-<N>` to `baseline/golden_std_output`:

| Outcome | How to identify |
|---------|----------------|
| **Masked** | `std_outputfile` is identical to `golden_std_output`; `errorfile` absent |
| **SDC** (Silent Data Corruption) | `std_outputfile` differs from `golden_std_output`; `errorfile` absent |
| **Crash** | `errorfile` contains "Program crashed" or a signal number (e.g. -11 = SIGSEGV) |
| **Hang** | `errorfile` contains "Program timed out" or similar timeout message |

Example `errorfile` for a crash:

```
Program crashed, terminated by the system, return code -11
```

Example `errorfile` for a hang:

```
Program timed out
```

For factorial with input 6, most runs will be masked (the program is small and
the fault often hits an unused register) or will produce a different number
(SDC).  Crashes are rare because the program does no pointer arithmetic.

### 8.3 Quick manual comparison

```bash
# Compare all runs against the golden output
for f in llfi/std_output/std_outputfile-run-0-*; do
    echo -n "$f: "
    if diff -q "$f" llfi/baseline/golden_std_output > /dev/null 2>&1; then
        echo "MASKED"
    else
        echo "SDC or CRASH — check llfi/error_output/"
    fi
done
```

---

## 9. Reproducing a specific run

If run 0-2 produced an interesting result and you want to reproduce it exactly,
copy the stat fields from `llfi.stat.fi.injectedfaults.0-2.txt` back into
`input.yaml`:

```yaml
runOption:
    - run:
        numOfRuns: 1
        fi_type: bitflip
        fi_cycle: 47
        fi_index: 12
        fi_reg_index: 0
        fi_bit: 28
```

Then re-run `injectfault`.  The runtime reads these pin values and injects at
exactly the same point.

---

## 10. Next steps

- **Try a different selector**: change `insttype: include: [all]` to
  `insttype: include: [fadd, fmul]` to target only floating-point arithmetic.
- **Try a software fault**: see `sample_programs/cpp_sample_programs/memcpy1/`
  for a `BufferOverflow(API)` example.
- **Try an ML model**: see `docs/tutorial_ml_experiment.md` for a complete
  walkthrough of layer-targeted fault injection on a TensorFlow/ONNX model.
- **Add tracing**: set `tracingPropagation: True` and re-run to generate
  per-instruction value traces in `llfi/llfi_stat_output/`.
- **Read the architecture**: `architecture.md` explains how the pass pipeline,
  selectors, and runtime library fit together.
