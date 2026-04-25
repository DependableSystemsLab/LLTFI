# LLTFI `input.yaml` Guide

Every program you run through LLTFI needs an `input.yaml` file in its working
directory.  This file controls which instructions are targeted for fault
injection (compile time) and how many runs are performed with which fault type
(run time).

The reference schemas for all keys are in:

- `docs/input_masterlist.yaml` — C/C++ programs
- `docs/input_masterlist_ml.yaml` — ML programs (ONNX-MLIR compiled)

This guide explains the keys in prose and provides annotated examples.
For a complete end-to-end walkthrough see `docs/tutorial_first_experiment.md`
(C/C++ programs) or `docs/tutorial_ml_experiment.md` (ML/ONNX models).


## Top-level structure

```yaml
defaultTimeOut: <seconds>   # optional; default 500 s

kernelOption:               # optional
    - forceRun

compileOption:              # required
    instSelMethod: ...
    regSelMethod: ...
    # optional tracing settings

runOption:                  # required; list of one or more run blocks
    - run:
        ...
```

---

## `defaultTimeOut`

Wall-clock timeout (seconds) applied to every fault injection run.  Individual
`run` blocks may override this with their own `timeOut` key.

```yaml
defaultTimeOut: 1000
```

---

## `kernelOption`

Optional.  Currently only one value is meaningful:

| Value | Effect |
|-------|--------|
| `forceRun` | Run the program even if profiling detects zero injectable instructions.  Useful when the target kernel is very short or when using a software fault selector that does not match every run. |

```yaml
kernelOption:
    - forceRun
```

---

## `compileOption`

Controls how `instrument.py` selects instructions and registers to mark as
fault injection targets.

### `instSelMethod`

A list with **exactly one** selector entry.  Three selector kinds are
available.

#### `insttype` — select by LLVM IR instruction type

```yaml
instSelMethod:
  - insttype:
      include:
        - all       # special keyword: every instruction type
      exclude:
        - ret       # always recommended — injecting into ret corrupts the stack
        - alloca
        - call
```

`include` and `exclude` take LLVM IR instruction names (lower-case mnemonic as
they appear in `.ll` files).  Common values:

| Name | Instruction |
|------|-------------|
| `add`, `sub`, `mul` | Integer arithmetic |
| `fadd`, `fsub`, `fmul`, `fdiv` | Floating-point arithmetic |
| `load`, `store` | Memory access |
| `getelementptr` | Address calculation |
| `icmp`, `fcmp` | Comparison |
| `call` | Function call |
| `ret` | Return (avoid injecting here) |
| `phi` | PHI node |
| `alloca` | Stack allocation (avoid) |

The full list is the LLVM Language Reference: https://llvm.org/docs/LangRef.html

#### `funcname` — select by containing function name

Only instruments instructions that are (or are not) inside a named function.
Combine with `insttype` semantics: `all` in `include` means every function.

```yaml
instSelMethod:
  - funcname:
      include:
        - all
      exclude:
        - main
        - helper_init
```

#### `customInstselector` — use a named selector plugin

Used for software fault modes and ML layer targeting.  The `include` list names
the selector class; `options` passes arguments to it.

```yaml
instSelMethod:
  - customInstselector:
      include:
        - BufferOverflow(API)
```

For ML programs (see [ML selectors](#ml-programs-customtensoroperator) below):

```yaml
instSelMethod:
  - customInstselector:
      include:
        - CustomTensorOperator
      options:
        - -layerNo=0
        - -layerName=conv
```

For pinning to specific LLFI indices (useful for reproducing a previous result):

```yaml
instSelMethod:
  - customInstselector:
      include:
        - llfiindex
      options:
        - -injecttoindex=2293
        - -injecttoindex=568
```

---

### `regSelMethod`

Selects which register within each targeted instruction to corrupt.

#### `regloc` — location-based selection

```yaml
regSelMethod: regloc
regloc: dstreg     # destination register (output of the instruction)
```

| `regloc` value | Meaning |
|----------------|---------|
| `dstreg` | Destination (output) register |
| `srcreg1` | First source register |
| `srcreg2` | Second source register |
| `srcreg3` | Third source register |
| `allreg` | All registers of the instruction |

`dstreg` is the most common choice for hardware fault experiments.  `allreg`
increases the injection surface.

#### `customregselector` — use a named register selector plugin

Required for software fault modes that have a paired register selector (most do):

```yaml
regSelMethod: customregselector
customRegSelector: BufferOverflow(API)
```

When using a `customInstselector`, the `customregselector` name should match.

---

### `includeInjectionTrace` (optional)

Expands the injection target set to include data-flow dependents of the
originally selected instructions.

```yaml
includeInjectionTrace:
    - forward    # instructions that consume the selected instruction's output
    - backward   # instructions whose output feeds into the selected instruction
```

Including the trace increases fault coverage at the cost of a larger injection
set (more possible injection points, longer profiling).

---

### `tracingPropagation` (optional)

Enables dynamic value tracing during fault injection runs, writing trace files
to `llfi/llfi_stat_output/`.

```yaml
tracingPropagation: True

tracingPropagationOption:
    maxTrace: 250      # max instructions recorded per run
    debugTrace: False  # print trace to stderr during run
    generateCDFG: True # write a dot-format control/data-flow graph
    mlTrace: False     # use ML-aware trace format (ML programs only)
```

Tracing adds overhead.  Disable it for large experiments where you only need
the pass/fail outcome.

---

## `runOption`

A list of one or more `run` blocks.  Each block is an independent experiment
that runs after the single profiling pass.  All blocks use the same
instrumented binary.

### Common keys

| Key | Type | Meaning |
|-----|------|---------|
| `numOfRuns` | int | Number of fault injection trials |
| `fi_type` | string | Fault type (see below) |
| `timeOut` | int | Per-run timeout in seconds; overrides `defaultTimeOut` |
| `verbose` | bool | Print return-code summary after each run |

### `fi_type` values

**Hardware faults:**

| Value | Effect |
|-------|--------|
| `bitflip` | Flip a randomly chosen bit in the register |
| `stuck_at_0` | Force all bits to 0 |
| `stuck_at_1` | Force all bits to 1 |

**Software faults** — use the selector name:

```yaml
fi_type: BufferOverflow(API)
```

See [Software fault modes](#software-fault-modes) for the full list.

**Auto-injection** — let the runtime choose the injector:

```yaml
fi_type: AutoInjection
```

---

### Pinning a fault (reproducibility)

To reproduce a specific previous injection, pin the exact cycle, register, and
bit.  All four keys must be present together:

```yaml
- run:
    numOfRuns: 1
    fi_type: bitflip
    fi_cycle: 684347      # dynamic instruction count at injection point
    fi_index: 417         # LLFI index of the targeted instruction
    fi_reg_index: 0       # which register of that instruction (0-based)
    fi_bit: 15            # which bit to flip (0-based from LSB)
```

Additional pin keys (less commonly needed):

| Key | Meaning |
|-----|---------|
| `fi_reg` | Raw register identifier (internal) |
| `fi_reg_pos` | Position within a multi-word register |

---

### Multiple-bit faults

Flip more than one bit within a single register:

```yaml
- run:
    numOfRuns: 5
    fi_type: bitflip
    fi_num_bits: 4    # flip 4 randomly chosen bits in one register
```

---

### Two-fault experiments (`window_len`)

Inject into two different registers with a bounded gap between them:

```yaml
- run:
    numOfRuns: 50
    fi_type: bitflip
    window_len: 10    # max dynamic instructions between the two injections
```

---

### Multiple faults across registers (`fi_max_multiple`)

Inject up to N faults into separate registers, with a random spacing drawn from
`[window_len_multiple_startindex, window_len_multiple_endindex]` dynamic
instructions between consecutive injections:

```yaml
- run:
    numOfRuns: 5
    fi_type: bitflip
    fi_max_multiple: 4
    window_len_multiple_startindex: 10
    window_len_multiple_endindex: 100
```

These keys can be combined with pin keys to anchor the first injection and then
spread subsequent ones randomly.

---

## ML programs: `CustomTensorOperator`

When injecting into an ONNX-MLIR compiled model, use `CustomTensorOperator` as
the instruction selector.  This selector understands the model's layer
structure.

```yaml
instSelMethod:
  - customInstselector:
      include:
        - CustomTensorOperator
      options:
        - -layerNo=<N>
        - -layerName=<type>
```

### `layerNo`

| Value | Meaning |
|-------|---------|
| `0` | All layers of the given type |
| `1` | First layer of the given type |
| `2` | Second layer, and so on |

### `layerName`

Valid layer type names:

`conv`, `relu`, `maxpool`, `matmul`, `add`, `avgpool`, `loop`,
`nonmaxs`, `unsqueeze`, `softmax`, `all`

Use `all` with `layerNo=0` to target every instruction in the model.

### Targeting multiple layer types

Separate multiple entries with `;`:

```yaml
options:
    - -layerNo=0;0;0
    - -layerName=conv;relu;matmul
```

The `layerNo` and `layerName` lists must have the same length.

---

## Software fault modes

Software faults use a `customInstselector` at compile time and a matching
`fi_type` at run time.  Both use the same name string, e.g. `BufferOverflow(API)`.

### Available modes (FIDL-generated)

| Class | Mode name |
|-------|-----------|
| **API** | `BufferOverflow(API)`, `BufferUnderflow(API)`, `InappropriateClose(API)`, `IncorrectOutput(API)`, `NoClose(API)`, `NoOpen(API)`, `NoOutput(API)`, `WrongAPI(API)`, `WrongMode(API)` |
| **Data** | `BufferOverflowMalloc(Data)`, `BufferOverflowMemmove(Data)`, `DataCorruption(Data)`, `WrongDestination(Data)`, `WrongPointer(Data)`, `WrongSource(Data)` |
| **IO** | `WrongRetrievedAddress(IO)`, `WrongRetrievedFormat(IO)`, `WrongSavedAddress(IO)`, `WrongSavedFormat(IO)` |
| **MPI** | `DeadLock(MPI)`, `InvalidMessage(MPI)`, `InvalidSender(MPI)`, `NoAck(MPI)`, `NoDrain(MPI)`, `NoMessage(MPI)`, `PacketStorm(MPI)` |
| **Res** | `CPUHog(Res)`, `DeadLock(Res)`, `InvalidPointer(Res)`, `LowMemory(Res)`, `MemoryExhaustion(Res)`, `MemoryLeak(Res)`, `StalePointer(Res)`, `ThreadKiller(Res)`, `UnderAccumulator(Res)`, `RaceCondition(Res)` |
| **Timing** | `HighFrequentEvent(Timing)` (hand-written) |

> **Note:** Fault modes that target `memmove`/`memcpy` by call site (e.g.
> `WrongDestination(Data)`, `BufferOverflowMemmove(Data)`) do not work when the
> compiler lowers those calls to LLVM intrinsics (`@llvm.memmove.*`).  Use API
> or IO modes (which target `fread`/`fwrite`) instead.

For a software fault experiment, both `instSelMethod` and `regSelMethod` should
name the same selector, and `fi_type` should match:

```yaml
compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - BufferOverflow(API)
    regSelMethod: customregselector
    customRegSelector: BufferOverflow(API)

runOption:
    - run:
        numOfRuns: 5
        fi_type: BufferOverflow(API)
```

---

## Complete examples

### Minimal hardware fault experiment

```yaml
compileOption:
    instSelMethod:
      - insttype:
          include:
            - all
          exclude:
            - ret

    regSelMethod: regloc
    regloc: dstreg

runOption:
    - run:
        numOfRuns: 100
        fi_type: bitflip
```

### Hardware fault with tracing and multiple run configurations

```yaml
defaultTimeOut: 1000

compileOption:
    instSelMethod:
      - insttype:
          include:
            - fadd
            - fmul
            - fdiv
          exclude:
            - ret

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: True
    tracingPropagationOption:
        maxTrace: 250
        debugTrace: False
        generateCDFG: False

runOption:
    - run:
        numOfRuns: 100
        fi_type: bitflip

    - run:
        numOfRuns: 50
        fi_type: stuck_at_0

    - run:
        numOfRuns: 50
        fi_type: stuck_at_1
```

### Software fault experiment

```yaml
defaultTimeOut: 500

kernelOption:
    - forceRun

compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - BufferOverflow(API)

    regSelMethod: customregselector
    customRegSelector: BufferOverflow(API)

runOption:
    - run:
        numOfRuns: 10
        fi_type: BufferOverflow(API)
```

### ML model — all convolutional layers, multiple faults per run

```yaml
defaultTimeOut: 5000

compileOption:
    instSelMethod:
      - customInstselector:
          include:
            - CustomTensorOperator
          options:
            - -layerNo=0
            - -layerName=conv

    regSelMethod: regloc
    regloc: dstreg

    includeInjectionTrace:
        - forward

    tracingPropagation: False
    tracingPropagationOption:
        maxTrace: 250
        debugTrace: False
        mlTrace: False
        generateCDFG: False

runOption:
    - run:
        numOfRuns: 1000
        fi_type: bitflip
        fi_max_multiple: 2
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
```

### ML model — all layer types

```yaml
defaultTimeOut: 5000

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
        fi_max_multiple: 2
        window_len_multiple_startindex: 1
        window_len_multiple_endindex: 500
        timeOut: 5000
```
