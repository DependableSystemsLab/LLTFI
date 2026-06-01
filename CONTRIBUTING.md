# Contributing to LLTFI

## Prerequisites

Before writing any code, read:
- [CODING_GUIDELINES.md](CODING_GUIDELINES.md) — C++ and Python style rules
- [CLAUDE.md](CLAUDE.md) — project architecture, build setup, constraints
- [caveats.txt](caveats.txt) — known limitations and gotchas

## Development Setup

```bash
# Run from the repo root. The setup script runs cmake and make internally.
./setup -LLFI_BUILD_ROOT /path/to/LLTFI-build \
        -LLVM_SRC_ROOT /path/to/llvm-project \
        -LLVM_DST_ROOT /usr/lib/llvm-20 \
        -LLVM_GXX_BIN_DIR /usr/lib/llvm-20/bin
```

After code changes, rebuild without re-running setup:
```bash
cd /path/to/LLTFI-build && make
```

## Making Changes

### C++ Changes

- Target C++17; compile against LLVM 20 APIs
- Use `nullptr`, `#ifndef` include guards, RAII memory management
- LLVM 20 API: `arg_size()` (not `getNumArgOperands()`), `#include "llvm/IR/CFG.h"` (not `llvm/Support/CFG.h`), `InsertPosition` via `.getIterator()` for instruction insertion points, `--passes=name` (not `-name`) for opt invocation
- `runOnModule` must return `bool`

### Python Changes

- Python 3 only; follow PEP 8
- Always use `with open(...)` for file I/O
- Never use `shell=True` in subprocess calls
- Use `sys.exit()`, not `exit()`
- Use `yaml.safe_load()`, not `yaml.load()`
- Minimum `except Exception:` — never bare `except:`

## Adding a Test Case

See `docs/adding_a_test.md` for a step-by-step guide to creating a new
regression test, including how to register a program, structure the test case
directory, and use the SKIP convention for optional dependencies.

## Running Tests

From the build directory:

```bash
# All core tests (must pass before any PR)
./test_suite/SCRIPTS/llfi_test --all_cpp

# Specific subsets
./test_suite/SCRIPTS/llfi_test --all_hardware_faults
./test_suite/SCRIPTS/llfi_test --all_trace_tools_tests

# Single test case
./test_suite/SCRIPTS/llfi_test --test_cases <TestName>
```

### ML/ONNX tests

```bash
./test_suite/SCRIPTS/llfi_test --all_ml
```

These are not part of `--all` because they require optional dependencies.
Tests with missing deps report **SKIP**, not FAIL, and don't affect the pass/fail count.

| Group | Requirements |
|-------|-------------|
| ML tool unit tests | `pip install onnx pygraphviz pydot` |
| TensorFlow → ONNX | `pip install tensorflow tf2onnx onnx` |
| PyTorch → ONNX | `pip install torch onnx` |
| ONNX → LLVM IR | onnx-mlir binary (`ONNX_MLIR_BUILD` env var) |
| Fault injection (ML) | LLTFI build + `model.ll` from `sample_programs/.../mnist/compile.sh` |

All core tests (`--all_cpp`) must pass before submitting a pull request. ML tests (`--all_ml`) should pass for any change that touches ML-related code.

## Pull Request Checklist

- [ ] All tests pass (`llfi_test --all_cpp`)
- [ ] Code follows CODING_GUIDELINES.md
- [ ] If changing a public API or behavior: README.md is updated
- [ ] New functionality has a corresponding test case
