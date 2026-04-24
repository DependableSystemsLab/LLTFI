# LLTFI Architecture

LLTFI is an LLVM-based fault injection framework for C/C++ and ML applications.
It is split into two completely independent layers that run at different times:
the **compile-time layer** (LLVM passes that transform IR) and the
**runtime layer** (a C library linked into the instrumented binary). These layers
communicate through a small set of well-defined interfaces: LLVM metadata embedded
in the IR, a config file written by the driver scripts, and log files written by
the runtime.

---

## High-Level Workflow

```
 Source code / LLVM IR
        │
        ▼
 ┌─────────────────────────────────────────────────────────┐
 │  bin/instrument.py   (reads input.yaml)                 │
 │    invokes opt with --passes="genllfiindexpass,         │
 │    profilingpass,faultinjectionpass[,insttracepass]"    │
 └──────────────────────────┬──────────────────────────────┘
                            │
          ┌─────────────────▼─────────────────┐
          │        llfi-passes.so             │
          │   (LLVM pass plugin – see §2)     │
          └─────────────────┬─────────────────┘
                            │ produces instrumented IR
                            ▼
             ┌──────────────────────────┐
             │   Instrumented binary    │
             │  (IR linked against      │
             │   libllfi-rt.so – §3)   │
             └──────┬───────────┬───────┘
                    │           │
          ┌─────────▼──┐  ┌─────▼──────────────┐
          │ prof.exe   │  │ fi.exe             │
          │ (profiling)│  │ (fault injection)  │
          └─────────┬──┘  └─────┬──────────────┘
                    │           │
          llfi.stat.prof.txt   llfi.stat.fi.injectedfaults.txt
          (opcode counts,      llfi.stat.trace.txt (optional)
           ML layer timings)   prog_output/
```

The driver scripts (`profile.py`, `injectfault.py`) orchestrate execution and
collect results. `tracediff.py` and related tools in `tools/` perform
post-processing.

---

## 1. Repository Layout

```
llvm_passes/            Compile-time: LLVM pass plugin source
  core/                   Pass infrastructure and selector framework
  hardware_failures/      Built-in hardware fault instruction selectors
  software_failures/      Software fault selectors (hand-written + FIDL-generated)
  instruction_duplication/ SID pass (SEDPasses.so)
  RegisterPasses.cpp      New-PM plugin entry point for llfi-passes.so
  CustomTensorOperatorInstSelector.cpp  ML-specific inst selector
  MainGraphInstSelector.cpp             ML-specific inst selector

runtime_lib/            Runtime: C/C++ library linked into instrumented binaries
  FaultInjectionLib.c     Core fault injection (preFunc, injectFunc)
  ProfilingLib.cpp        Opcode profiling (doProfiling, endProfiling)
  InstTraceLib.c          Instruction tracing (printInstTracer)
  FaultInjectorManager.h/cpp  Plugin registry for custom fault injectors
  MLFaultInjectionLib.cpp ML layer tracking (lltfiMLLayer)

bin/                    Driver scripts
  instrument.py           Compiles IR with LLFI passes; produces prof.exe + fi.exe
  profile.py              Runs prof.exe; collects opcode frequencies
  injectfault.py          Runs fi.exe repeatedly with configured fault parameters
  batchInstrument.py      Runs instrument.py across multiple programs in one call
  batchProfile.py         Runs profile.py across multiple programs
  batchInjectfault.py     Runs injectfault.py across multiple programs
  HardwareFailureAutoScan.py  Lists applicable hardware selectors for a program
  SoftwareFailureAutoScan.py  Lists applicable software fault modes for a program
  InjectorAutoScan.py     Lists all registered fault injector names (fi_type values)
  llfi-gui.py             Launches the LLFI graphical front-end

tools/                  Post-processing and ML utilities
  FIDL/                   Software fault mode code generator (§4)
  GenerateMakefile/       Test harness Makefile generator
  tracediff.py            Compares golden vs faulty instruction traces
  traceontograph.py       Overlays traces onto dependency graph
  ExtendONNXModel.py      Prepares ONNX model for LLTFI instrumentation
  outputONNXGraph.py      Visualises ONNX model graph
  compiletoIR.py          Converts source to LLVM IR

docs/                   Reference schemas and user guides
  tutorial_first_experiment.md  End-to-end C/C++ walkthrough and output interpretation
  tutorial_ml_experiment.md     End-to-end ML/ONNX walkthrough (layer targeting, multi-fault)
  adding_a_test.md        How to add a regression test case
  input_yaml_guide.md     Prose guide to writing input.yaml (start here)
  input_masterlist.yaml   Full key reference for input.yaml (C/C++ programs)
  input_masterlist_ml.yaml Full key reference for ML programs

test_suite/             Regression tests
```

---

## 2. Compile-Time Layer: LLVM Pass Plugin

Everything in `llvm_passes/` runs at compile time, inside `opt`. It is built into
two shared libraries:

| Library | Contents |
|---------|----------|
| `llfi-passes.so` | All core passes, all hardware and software fault selectors |
| `SEDPasses.so` | Selective Instruction Duplication (ML only) |

### 2.1 Pass Pipeline

The standard instrumentation pipeline runs three passes in sequence.
`instrument.py` invokes `opt` as:

```
opt -load-pass-plugin llfi-passes.so \
    --passes="genllfiindexpass,profilingpass,faultinjectionpass" \
    [selector options] input.ll -o output.ll
```

The passes must run in this order because each depends on the previous:

```
GenLLFIIndexPass
    Assigns a unique integer (the LLFI index) to every instruction in the
    module.  The index is stored as LLVM metadata on the instruction and
    read at runtime to identify the instruction being executed.
    Output file: llfi.stat.totalindex.txt
         │
         ▼
ProfilingPass
    Inserts a call to doProfiling(opcode) before each FI-candidate
    instruction.  Also inserts endProfiling() at program exit points.
    For ML models, inserts lltfiMLLayer() calls around OMInstrumentPoint
    boundaries to track per-layer timing.
         │
         ▼
FaultInjectionPass
    Inserts preFunc() / injectFunc() calls around each FI-candidate
    register.  The selector framework (§2.2) determines which instructions
    and registers are candidates.
```

Optional passes can be added:

```
InstTracePass
    Inserts printInstTracer() calls to record register values at runtime.
    Used for trace-based analysis with tracediff.py.

LLFIDotGraphPass  (registered as "dotgraphpass")
    Generates llfi.stat.graph.dot, a Graphviz data-dependency graph of the
    module.  Added to the pass list when genDotGraph: true is set in
    input.yaml.  Used by the zgrviewer-based graph viewer.
```

Two additional passes are used only by the auto-scan scripts and not in the
normal instrumentation pipeline:

```
HardwareFailureAutoScanPass   Writes llfi.applicable.hardware.selectors.txt
SoftwareFailureAutoScanPass   Writes llfi.applicable.software.failures.txt
```

### 2.2 Instruction and Register Selector Framework

The selector framework decides *which* instructions and *which* register
positions within those instructions to inject faults into. It is the central
extensibility point of LLTFI.

#### Class Hierarchy

```
FIInstSelector  (llvm_passes/core/FIInstSelector.h)
│   virtual bool isInstFITarget(Instruction*) = 0
│   virtual void getCompileTimeInfo(map<string,string>&)
│   void getFIInsts(Module&, set<Instruction*>*)  — calls isInstFITarget
│
├── HardwareFIInstSelector
│   │   (for hardware fault modes — any instruction may be the target)
│   ├── InstTypeFIInstSelector          by opcode (fadd, load, store, …)
│   ├── FuncNameFIInstSelector          all insts in a named function
│   ├── LLFIIndexFIInstSelector         one specific LLFI index
│   ├── CustomTensorOperatorInstSelector  ONNX operator boundary (ML)
│   └── MainGraphInstSelector           all arith insts in main_graph (ML)
│
└── SoftwareFIInstSelector
    │   (for software fault modes — typically targets call instructions)
    ├── [35+ FIDL-generated selectors]  _Data_*, _API_*, _MPI_*, …
    └── [_Timing_HighFrequentEventInstSelector]  hand-written

FIRegSelector  (llvm_passes/core/FIRegSelector.h)
│   virtual bool isRegofInstFITarget(Value*, Instruction*) = 0
│   void getFIInstRegMap(set<Instruction*>, map<Instruction*, list<int>>*)
│
├── HardwareFIRegSelector
│   └── RegLocBasedFIRegSelector   dstreg / srcreg1–4 / allreg / allsrcreg
│
└── SoftwareFIRegSelector
    ├── FuncArgRegSelector    argument registers of a call
    ├── FuncDestRegSelector   destination register of a call
    └── RetValRegSelector     return value register
```

#### Controller

`Controller` (singleton, `llvm_passes/core/Controller.h`) is the glue between
the pass pipeline and the selector framework. It parses LLVM command-line
options set by `instrument.py`, instantiates the appropriate selector objects,
and exposes the final `fi_inst_regs_map` (a mapping of `Instruction*` →
`list<int>` register positions) to `ProfilingPass` and `FaultInjectionPass`.

Key options it parses:

| Option | Values | Meaning |
|--------|--------|---------|
| `-fiinstselmethod` | `insttype`, `funcname`, `custominstselector` | Which inst selector to use |
| `-includeinst` / `-excludeinst` | opcode names | Filter by instruction type |
| `-includefunc` / `-excludefunc` | function names | Filter by function |
| `-fiinstselectorname` | selector name string | Used with `custominstselector` |
| `-firegselmethod` | `regloc`, `customregselector` | Which reg selector to use |
| `-fireglocation` | `dstreg`, `srcreg1`–`srcreg4`, `allreg`, `allsrcreg` | Which register position |

#### Custom Selector Manager

`FICustomInstSelectorManager` and `FICustomRegSelectorManager` are singleton
registries. Hardware and software selectors register themselves at static
initialisation time via:

```cpp
static RegisterFIInstSelector X("mymode", new MyModeInstSelector());
static RegisterFIRegSelector  Y("mymode", new MyModeRegSelector());
```

The manager resolves a name string (e.g. `"insttype"`, `"WrongPointer(Data)"`)
to a concrete selector object at instrumentation time.

### 2.3 Hardware Fault Selectors

Hardware fault selectors model low-level physical faults (bit-flips, stuck-at
bits) that can affect any instruction. They are built into `llfi-passes.so`
and are always available.

| Selector name | Class | Targets |
|---------------|-------|---------|
| `insttype` | `InstTypeFIInstSelector` | All instructions matching a set of LLVM opcodes |
| `funcname` | `FuncNameFIInstSelector` | All instructions in one or more named functions |
| `llfiindex` | `LLFIIndexFIInstSelector` | The single instruction with a given LLFI index |
| `maingraph` | `MainGraphInstSelector` | FAdd / FMul / FCmp in `main_graph()` (ML) |
| `CustomTensorOperator` | `CustomTensorOperatorInstSelector` | FP arith inside a named ONNX operator region (ML) |

Register selection for all hardware selectors is handled by
`RegLocBasedFIRegSelector`, controlled by the `-fireglocation` option.

The `HardwareFailureAutoScanPass` (invoked by `HardwareFailureAutoScan.py`)
enumerates all registered hardware selectors and writes the list to
`llfi.applicable.hardware.selectors.txt`.

### 2.4 Software Fault Selectors

Software fault selectors model high-level software bugs (wrong API usage,
memory corruption, timing errors). Unlike hardware selectors, each software
fault mode pairs an instruction selector with a custom register selector and
often a custom fault injector.

#### Architecture of a Software Fault Mode

Each software fault mode consists of three parts registered under the same name:

```
  Instruction selector   — which call instruction to target
  Register selector      — which argument/return register to perturb
  Fault injector         — how to perturb it (bitflip, sleep, override value, …)
```

#### FIDL-Generated Selectors

The majority of software fault modes are generated by the FIDL tool (see §4)
from `tools/FIDL/config/default_failures.yaml`. The generated files live in
`llvm_passes/software_failures/` and are excluded from git (they are
regenerated by `./setup`). There are 37 generated fault modes organised into
six categories:

| Category | Example modes |
|----------|---------------|
| `Data` | `DataCorruption`, `WrongSource`, `WrongDestination`, `WrongPointer`, `BufferOverflowMalloc`, `BufferOverflowMemmove` |
| `API` | `BufferOverflow`, `BufferUnderflow`, `WrongAPI`, `WrongMode`, `NoOpen`, `NoClose`, `InappropriateClose`, `NoOutput`, `IncorrectOutput` |
| `MPI` | `NoMessage`, `InvalidMessage`, `InvalidSender`, `NoAck`, `NoDrain`, `DeadLock`, `PacketStorm` |
| `Resource` | `LowMemory`, `MemoryExhaustion`, `MemoryLeak`, `InvalidPointer`, `StalePointer`, `DeadLock`, `ThreadKiller`, `CPUHog`, `UnderAccumulator` |
| `IO` | `WrongSavedFormat`, `WrongRetrievedFormat`, `WrongSavedAddress`, `WrongRetrievedAddress` |
| `Timing` | `RaceCondition` |

#### Hand-Written Software Fault Selectors

Two files are tracked in git and not generated by FIDL:

- `_SoftwareFaultRegSelectors.h/cpp` — defines `FuncArgRegSelector`,
  `FuncDestRegSelector`, `RetValRegSelector` (the register-level counterparts
  used by many FIDL-generated instruction selectors)
- `_Timing_HighFrequentEventSelector.cpp` — a complete hand-written fault mode
  (`HighFrequentEvent(Timing)`) that targets `fread`/`fopen`/`fwrite` calls
  and return instructions

The `SoftwareFailureAutoScanPass` (invoked by `SoftwareFailureAutoScan.py`)
runs all registered software selectors against the module IR and writes
`llfi.applicable.software.failures.txt` listing only the modes that actually
match instructions in the program.

#### Known Limitation: memmove/memcpy Intrinsics

Software fault modes that target `memmove` or `memcpy` by call site (e.g.
`WrongDestination(Data)`, `BufferOverflowMemmove`) do not work when the
compiler lowers those calls to LLVM intrinsics
(`@llvm.memmove.p0.p0.i64`, `@llvm.memcpy.p0.p0.i64`). Intrinsics have no
injectable register arguments at the call site that LLTFI can intercept at
runtime. To inject software faults into memory operations in such programs,
use fault modes that target regular C library calls instead (e.g.
`WrongPointer(Data)` targeting `fread`/`fwrite`).

### 2.5 ML Fault Selectors

ML fault injection operates on LLVM IR compiled from ONNX models via onnx-mlir.
The onnx-mlir compiler annotates the IR with `@OMInstrumentPoint(operator_id, flag)`
calls that delimit each tensor operator's computation region. LLTFI's ML
selectors use these boundaries to confine fault injection to a specific layer.

#### CustomTensorOperatorInstSelector

Registered as `"CustomTensorOperator"`. Selects `FAdd`, `FSub`, `FMul`,
`FDiv`, and `FCmp` instructions inside `main_graph()` that fall between a
matching `OMInstrumentPoint(id, 2)` (start) and `OMInstrumentPoint(id, 1)`
(end) pair. The operator is identified by name (e.g. `conv`, `relu`, `matmul`)
mapped to ONNX operator IDs.

Command-line options (set via `input.yaml`):
- `--layerName=conv;relu` — target these operator types
- `--layerNo=2;0` — target the 2nd `conv` and all `relu` occurrences (0 = all)

#### MainGraphInstSelector

Registered as `"maingraph"`. Simpler selector that targets all `FAdd`, `FMul`,
and `FCmp` instructions anywhere in `main_graph()`, without operator-boundary
awareness. Used when operator-level granularity is not required.

### 2.6 Selective Instruction Duplication (SEDPasses.so)

The `InstructionDuplicationPass` in `SEDPasses.so` is a separate pass plugin
for soft-error detection and correction in ML models. It is not part of the
normal fault injection pipeline — it is applied to the model IR *before*
instrumentation.

For each selected arithmetic instruction, the pass:
1. Duplicates the instruction (clone + insert immediately after)
2. Inserts a call to `compareFloatValues(original, duplicate)` which returns
   the bitwise AND of both results
3. Replaces uses of the original result with the `compareFloatValues` return

When both copies agree (no fault), `compareFloatValues(x, x) == x` so the
result is unchanged. When they disagree (transient fault in one copy), the AND
masks the corrupted bits.

The pass supports two modes:
- **AID** (Arithmetic Instruction Duplication): each selected instruction
  duplicated independently
- **ACD** (Arithmetic Chain Duplication, `--enableChainDuplication`):
  consecutive arithmetic sequences duplicated as a unit and compared only at
  the end of the chain

`compareFloatValues` is defined in
`llvm_passes/instruction_duplication/shared_lib/SIDHelperFunctions.cpp` and
linked into the model IR via `llvm-link` before execution.

---

## 3. Runtime Layer: libllfi-rt

The runtime library is a set of C/C++ files compiled to a shared library
(`libllfi-rt.so`) and linked into every instrumented binary. It is
**completely independent of LLVM** — it runs inside the target program, not
inside `opt`.

The runtime reads its configuration from `llfi.config.runtime.txt` (written
by `injectfault.py` before each run) and writes its results to log files in
the `llfi/` output directory.

### 3.1 Fault Injection Runtime (FaultInjectionLib.c)

This is the core of the runtime layer. The instrumented IR calls two functions
around each fault-injection-candidate register:

```c
// Called before the instruction executes.
// Returns true if this dynamic instance should be injected, false otherwise.
bool preFunc(long llfi_index, unsigned opcode,
             unsigned my_reg_index, unsigned total_reg_target_num);

// Called after preFunc returns true.  Corrupts the register value in-place.
void injectFunc(long llfi_index, unsigned size, char *buf,
                unsigned my_reg_index, unsigned reg_pos, char *opcode_str);
```

**`preFunc` selection logic:**  
The runtime uses either *cycle-based* or *index-based* targeting (set in
`llfi.config.runtime.txt`). In cycle-based mode it counts every call across
all instructions and injects when the cycle counter matches `fi_cycle`. In
index-based mode it injects the first dynamic occurrence of the instruction
with the given LLFI index. A fault is injected at most once per instruction
execution (even when multiple registers are targeted).

**`injectFunc` fault types** (set by `fi_type` in config):

| `fi_type` | Operation |
|-----------|-----------|
| `bitflip` | XOR a randomly selected bit |
| `stuck_at_0` | AND the bit to force it to 0 |
| `stuck_at_1` | OR the bit to force it to 1 |
| Software injectors | Custom logic (sleep, wrong value, etc.) — see §3.2 |

After injection, the runtime appends a record to
`llfi.stat.fi.injectedfaults.txt` with the LLFI index, register size, bit
position, and fault type.

**Config file keys** (`llfi.config.runtime.txt`):

| Key | Meaning |
|-----|---------|
| `fi_type` | Fault type (see table above) |
| `fi_cycle` | Inject at this dynamic instruction cycle |
| `fi_index` | Inject at this LLFI index (alternative to `fi_cycle`) |
| `fi_reg_index` | Target register index within the instruction (random if absent) |
| `fi_bit` | Target bit position (random if absent) |
| `fi_num_bits` | Number of bits to corrupt (default 1) |
| `fi_max_multiple` | Number of injection points for multi-fault experiments |
| `fi_next_cycle` | Additional cycles for multi-fault injection |

### 3.2 Fault Injector Plugin Registry (FaultInjectorManager)

Software fault modes that need custom injection logic (e.g. inserting a sleep,
returning a wrong value) register a `FaultInjector` subclass with the
singleton `FaultInjectorManager`. The manager resolves the injector name from
`fi_type` in the config file to the corresponding `injectFault()` implementation.

The runtime injector registrations live in two tracked files:

- `runtime_lib/CommonFaultInjectors.cpp` — the three hardware injectors
  (`bitflip`, `stuck_at_0`, `stuck_at_1`).
- `runtime_lib/_FIDLSoftwareFaultInjectors.cpp` — an aggregator that
  `#include`s the hand-written injector class definitions from
  `_SoftwareFaultInjectors.cpp` and then registers all 37 FIDL-named software
  injectors.  This file is tracked in git (unlike the selector `.cpp` files in
  `llvm_passes/software_failures/`) and must be updated manually when a new
  FIDL fault mode is added.

### 3.3 Profiling Runtime (ProfilingLib.cpp)

```c
void doProfiling(int opcode);   // Inserted before each FI-candidate inst
void endProfiling();            // Inserted at program exit
```

`doProfiling` increments a per-opcode counter weighted by an estimated cycle
cost. `endProfiling` writes `llfi.stat.prof.txt`:

```
total_cycle=<weighted sum across all opcodes>
```

For ML models, `MLFaultInjectionLib.cpp` provides:

```c
void lltfiMLLayer(int64_t layerName, int64_t start);
```

Called at each `OMInstrumentPoint` boundary, it records the start and end
cycle of each tensor operator. The profiling data drives `injectfault.py`'s
fault space sampling (which dynamic instruction cycles to target).

### 3.4 Instruction Trace Runtime (InstTraceLib.c)

```c
void printInstTracer(long instID, char *opcode, int size, char *ptr, int maxPrints);
```

Writes a line to `llfi.stat.trace.txt` for every call. In the *golden run*
(no fault) all instructions are traced. In the *fault injection run* tracing
is gated by a state machine: it starts after the fault is injected and records
the next `maxPrints` instructions. The delta between the two trace files is the
input to `tracediff.py`.

---

## 4. FIDL — Software Fault Code Generator

FIDL (Fault Injection Description Language) generates the software fault
selector `.cpp` files from a YAML description, avoiding the need to write
repetitive boilerplate for each fault mode.

```
tools/FIDL/config/default_failures.yaml   Fault mode specifications
tools/FIDL/FIDL-Algorithm.py              Generator script
tools/FIDL/config/TargetSingleTemplate.cpp     Template: single function target
tools/FIDL/config/TargetMultiSourceTemplate.cpp  Template: multi-arg target
tools/FIDL/config/TargetAllTemplate.cpp    Template: all instructions
tools/FIDL/config/NewInjectorTemplate.cpp  Template: custom fault injector
```

To regenerate after editing `default_failures.yaml`:
```bash
python3 tools/FIDL/FIDL-Algorithm.py -a default
```

This is run automatically by `./setup`. The generated `_*_*Selector.cpp` files
are listed in `.gitignore` — do not commit them.

Each YAML fault mode entry specifies:
- **`Failure_Class`** / **`Failure_Mode`** — naming (e.g. `Data` / `WrongPointer`)
- **`Trigger`** — which call to intercept (`call: [fread, fwrite]`, `return:`)
- **`Target`** — source arguments or destination register to perturb
- **`Action`** — how to perturb (`Bitflip: true` or custom C++ injector code)

FIDL selects the appropriate template based on the trigger/target combination
and substitutes the fault-mode-specific values.

---

## 5. Interface Between the Two Layers

The compile-time and runtime layers share three interfaces. There is no shared
header between them — the contract is purely by convention.

### 5.1 LLVM Metadata (compile-time → runtime)

`GenLLFIIndexPass` stores each instruction's LLFI index as LLVM metadata:

```cpp
// Written by GenLLFIIndexPass:
inst->setMetadata("llfi_index", MDNode::get(ctx, ConstantAsMetadata::get(
    ConstantInt::get(Type::getInt64Ty(ctx), index))));
```

`FaultInjectionPass` reads this metadata when generating the `preFunc` /
`injectFunc` calls, embedding the index as a constant argument. The runtime
never parses metadata — it receives the index as a plain integer argument.

### 5.2 Runtime Config File (driver → runtime)

`injectfault.py` writes `llfi.config.runtime.txt` immediately before launching
`fi.exe`. The runtime reads it at startup in `initInjections()`. No LLVM types
or headers are involved — it is a plain text key=value file.

### 5.3 Log Files (runtime → driver / post-processing tools)

All output files are written by the runtime to the `llfi/` directory created
by `instrument.py`:

```
llfi/
  llfi_stat_output/
    llfi.stat.totalindex.txt      Total instructions (GenLLFIIndexPass)
    llfi.stat.prof.txt            Profiling results (ProfilingLib)
    llfi.stat.fi.injectedfaults.txt  Injection log (FaultInjectionLib)
    llfi.stat.trace.*.txt         Instruction traces (InstTraceLib)
  std_output/                     Stdout from each run
  error_output/                   Stderr / crash info from each run
  prog_output/                    Disk output from faulty runs
  baseline/                       Golden-run output and trace
```

---

## 6. Adding a New Fault Mode

### New hardware fault mode (inst selector only)

1. Subclass `HardwareFIInstSelector` in a new `.cpp` under `llvm_passes/`:
   ```cpp
   class MySelector : public HardwareFIInstSelector {
     bool isInstFITarget(Instruction* inst) override { ... }
     void getCompileTimeInfo(map<string,string>& info) override { ... }
   };
   static RegisterFIInstSelector X("mymode", new MySelector());
   ```
2. Add the file to `llvm_passes/CMakeLists.txt` under the `llfi-passes` target.
3. Rebuild (`make` in the build root).

### New software fault mode via FIDL

1. Add an entry to `tools/FIDL/config/default_failures.yaml`.
2. Run `python3 tools/FIDL/FIDL-Algorithm.py -a default` and rebuild.
3. Update `expected_count` in `test_suite/SCRIPTS/test_fidl_generation.py`.

### New software fault mode (hand-written)

Follow the same pattern as `_Timing_HighFrequentEventSelector.cpp`:
- Define `_<Class>_<Mode>InstSelector : public SoftwareFIInstSelector`
- Define `_<Class>_<Mode>RegSelector : public SoftwareFIRegSelector`
- Register both with `RegisterFIInstSelector` / `RegisterFIRegSelector`
- Add to `CMakeLists.txt` explicitly (not caught by the FIDL gitignore pattern)

### New fault injector (runtime)

Subclass `FaultInjector` in `runtime_lib/` and register it:
```cpp
class MySoftwareInjector : public FaultInjector {
  void injectFault(long index, unsigned size, unsigned fi_bit,
                   char *buf) override { ... }
};
static RegisterFaultInjector R("MySoftwareInjector", new MySoftwareInjector());
```

Set `fi_type=MySoftwareInjector` in `input.yaml` to select it at runtime.

---

## 7. Key Design Decisions

**Selector registration at static init time.** Both inst and reg selectors
register themselves via `static RegisterFI*Selector` objects, which run before
`main()`. This means new selectors are available as soon as they are linked
into `llfi-passes.so` — no central registry to update, no switch statement to
extend.

**Two-phase runtime check.** `preFunc` and `injectFunc` are separate calls.
`preFunc` is cheap (counter comparison) and runs every time. `injectFunc` is
called only when `preFunc` returns true, keeping the hot path overhead minimal.

**LLFI index as the universal identifier.** Every instruction in the module gets
a unique stable integer at compile time. This index is the only way the
compile-time and runtime layers refer to the same instruction — no function
names, no IR text, no debug info dependency.

**FIDL-generated files are not committed.** They are regenerated deterministically
from the YAML spec by `./setup`. This keeps the repository free of large
amounts of repetitive generated code while still allowing the generated files
to be inspected locally after a build.

**ML instrumentation is non-invasive to the core.** The ML-specific selectors
(`CustomTensorOperatorInstSelector`, `MainGraphInstSelector`) are ordinary
`HardwareFIInstSelector` subclasses that happen to look for `OMInstrumentPoint`
calls. The core `FaultInjectionPass` and runtime are unchanged for ML workloads.

**New pass manager only (legacy PM removed).** The legacy `opt -load` /
`-enable-new-pm=0` interface was dropped in LLVM 17 and is no longer supported
in LLTFI. All passes — including `InstructionDuplication` — use the new pass
manager (`PassInfoMixin`, `llvmGetPassPluginInfo`). This removes the need to
maintain two registration paths for every pass and aligns with LLVM's own
direction for pass infrastructure.

**SEDPasses.so is separate from llfi-passes.so.** The
`InstructionDuplicationPass` lives in its own plugin so it can be applied to
the model IR *before* LLFI instrumentation. The SED transformation alters
instruction counts and structure; if it ran inside the LLFI pass pipeline (after
`GenLLFIIndexPass` has already assigned indices) those indices would be
invalidated. A separate library makes the mandatory pre-instrumentation ordering
explicit and prevents accidental composition in the wrong sequence.
