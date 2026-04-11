# LLTFI — Claude Context

## What this project is

LLTFI (Low-Level Tensor Fault Injector) is an LLVM-based fault injection framework supporting C/C++ and ML applications (TensorFlow, PyTorch). It injects faults into LLVM IR. It is built on top of LLFI and is fully backward compatible with it.

---

## Build setup (this machine)

| Variable | Path |
|----------|------|
| Source tree | `/home/karthik/Programs/LLTFI` |
| Build root | `/home/karthik/Programs/LLTFI-build` |
| LLVM DST root | `/usr/lib/llvm-15` (apt install) |
| LLVM SRC root | `/home/karthik/Programs/llvm-project` |
| LLVM version | 15.0 (commit `9778ec057cf4`) |

To rebuild from source:
```bash
./setup -LLFI_BUILD_ROOT /home/karthik/Programs/LLTFI-build \
        -LLVM_SRC_ROOT /home/karthik/Programs/llvm-project \
        -LLVM_DST_ROOT /usr/lib/llvm-15 \
        -LLVM_GXX_BIN_DIR /usr/lib/llvm-15/bin
```

The build root must not already exist. Delete it first if rebuilding from scratch.

To rebuild after code changes (faster):
```bash
cd /home/karthik/Programs/LLTFI-build && make
```

---

## Running the test suite

From the **build** directory:
```bash
cd /home/karthik/Programs/LLTFI-build/test_suite
python3 SCRIPTS/llfi_test --all                      # all 21 tests
python3 SCRIPTS/llfi_test --all_hardware_faults      # 8 tests
python3 SCRIPTS/llfi_test --all_software_faults      # 5 tests
python3 SCRIPTS/llfi_test --all_trace_tools_tests    # 3 tests
python3 SCRIPTS/llfi_test --all_makefile_generation  # 2 tests
python3 SCRIPTS/llfi_test --all_fidl                 # 3 tests (FIDL generator)
python3 SCRIPTS/llfi_test --all_ml                   # ML/ONNX tools + SoftwareFailureAutoScan
```

Expected: **21/21 PASS** for `--all`. Some error messages during fault injection runs are normal.

`--all_ml` runs additional tests not included in `--all`:

| Test group | Tests | Requirements |
|------------|-------|--------------|
| `SoftwareFailureAutoScan` | 4 | LLTFI build only |
| `CompareLayerOutputs` | 2 | `pip install onnx pygraphviz` |
| `ExtendONNXModel` | 1 | `pip install onnx` |
| `outputONNXGraph` | 1 | `pip install onnx pydot` |
| TensorFlow → ONNX | 3 | `pip install tensorflow tf2onnx onnx` |
| PyTorch → ONNX | 2 | `pip install torch onnx` |
| ONNX → LLVM IR | 2 | `onnx-mlir` + `mlir-translate` on PATH or `$ONNX_MLIR_BUILD` set |
| Fault injection (ML) | 3 | LLTFI build + `model.ll` in `sample_programs/.../mnist/` (run `compile.sh` first) |

Tests with missing deps are reported as **SKIP** (not FAIL) and excluded from the pass/fail count.

The ONNX→IR and fault injection tests use the pre-built `model.onnx` from
`sample_programs/ml_sample_programs/vision_models/mnist/`. The fault injection
test also requires `model.ll` which is produced by that directory's `compile.sh`.

---

## Key architecture

```
llvm_passes/          LLVM pass plugins (compiled to llfi-passes.so)
  core/               Fault injection, profiling, tracing passes
  hardware_failures/  Built-in hardware fault selectors (bitflip, funcname, etc.)
  software_failures/  Software fault selectors — hand-written and FIDL-generated
runtime_lib/          Runtime library (libllfi-rt.so) linked into instrumented binaries
bin/                  Python driver scripts: instrument.py, profile.py, injectfault.py
docs/                 input_masterlist.yaml, input_masterlist_ml.yaml — reference schemas
                      for the input.yaml files that control instrumentation and injection
tools/                Trace analysis tools (tracediff.py, traceontograph.py, traceunion.py,
                      tracetodot.py), FIDL/, GenerateMakefile/
test_suite/           Regression tests
  PROGRAMS/           Source programs used by tests
  HardwareFaults/     Hardware fault injection test cases
  SoftwareFaults/     Software fault injection test cases
  Traces/             Pre-committed trace reference files for trace tool tests
  MakefileGeneration/ Makefile generation test cases
```

---

## FIDL — software fault infrastructure

FIDL generates LLVM pass selector `.cpp` files from YAML fault mode descriptions:

```bash
python3 tools/FIDL/FIDL-Algorithm.py -a default
```

This reads `tools/FIDL/config/default_failures.yaml` and writes selector files to `llvm_passes/software_failures/`. **These generated files are in `.gitignore`** — do not commit them. `./setup` runs FIDL automatically before cmake.

If you add or modify fault modes in `default_failures.yaml`, re-run FIDL and rebuild. Edit templates in `tools/FIDL/config/` (not the generated files) for structural changes.

The three FIDL templates are:
- `TargetSingleTemplate.cpp` — targets a single function name
- `TargetMultiSourceTemplate.cpp` — targets multiple function args
- `TargetAllTemplate.cpp` — targets all instructions

---

## LLVM 15 API constraints

The codebase targets **LLVM 15**. Key API changes from older LLVM versions to keep in mind:

- `#include "llvm/IR/CFG.h"` (not `llvm/Support/CFG.h` — removed in LLVM 15)
- `CI->arg_size()` (not `CI->getNumArgOperands()` — removed in LLVM 15)
- `func->getName().str()` returns `StringRef`; call `.str()` before assigning to `std::string`
- New pass manager is used; legacy pass manager wrappers are kept for compatibility
- `opt -load-pass-plugin` (not `opt -load` — old syntax removed)

---

## Software fault injection — known limitation

Fault modes that target `memmove`/`memcpy` by function call (e.g. `WrongDestination(Data)`) do **not** work with programs where the compiler optimises those calls to LLVM intrinsics (`@llvm.memmove.p0.p0.i64`). The intrinsic has no injectable register arguments at runtime. Use fault modes targeting `fread`/`fwrite` or other regular C library calls instead (e.g. `WrongPointer(Data)`).

---

## Test suite — untracked files that should not be committed

The following files appear as untracked after running tests and should not be staged:
- `test_suite/HardwareFaults/*/inp.in`, `graph_input.dat` — deployed by `deploy_prog.py` from `PROGRAMS/`
- `test_suite/HardwareFaults/*/llfi.test.log.*` — test run artifacts
- `test_suite/Traces/*/llfi/llfi_stat_output/*.report.txt` — generated by trace analysis

---

## Code style

See `CODING_GUIDELINES.md` for the full style guide. Key points:

**C++:**
- Use `nullptr`, not `NULL`
- Every header needs `#ifndef` include guards
- Missing `return` in `bool runOnModule(...)` is UB — always return `false` unless IR was modified
- Use `except Exception:` in Python; in C++ use `errs()` for pass diagnostics

**Python:**
- Python 3 only (`#!/usr/bin/env python3`)
- Use `with open(...) as f:` always — no bare `open()` without context manager
- `except Exception:` minimum; use specific types (`OSError`, `KeyError`) where known
- `sys.exit()`, not `exit()`
- No `subprocess(..., shell=True)` — use list args and `stdout=open(file, 'w')` for redirection
- `yaml.safe_load()` always — never `yaml.load()` without Loader

---

## Wiki known inconsistencies (external, not fixable here)

- Wiki Page 4: says `input_masterlist.yaml` is in `bin/` — actually in `docs/`
- Wiki Page 5: uses `llvm-gcc` (replaced by clang) and old `opt -load` syntax
- Wiki Page 9: references `gui/config/` files that no longer exist (Java GUI removed)
