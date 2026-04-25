# Changelog

All notable changes to LLTFI are recorded here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased] — LLVM 20 branch (`LLVM20`)

This release upgrades LLTFI from LLVM 15 to LLVM 20.  Every change is
backward-compatible with LLFI.  The full migration narrative, task breakdown,
and effort accounting are in `migration.md`.

### Breaking changes

- **LLVM version requirement raised from 15 to 20.**  LLVM 15 is no longer
  supported.  Install LLVM 20 via the LLVM apt repository or build from source
  (see `README.md`).
- **Legacy pass manager (`opt -load`, `-enable-new-pm=0`) removed.**  All
  passes — including `InstructionDuplication` — now use the new pass manager
  exclusively.  Any external scripts calling `opt` directly must be updated to
  use `-load-pass-plugin` and `--passes=<PassName>`.
- **`InstructionDuplication` pass renamed** to `InstructionDuplicationPass` in
  the plugin registry to match the new PM convention.

---

### Added

#### New passes and tests
- `InstructionDuplication` migrated to the new pass manager (`PassInfoMixin`);
  exposed as `"InstructionDuplicationPass"` in `SEDPasses.so`.
- Two new tests in `test_instruction_duplication.py`:
  - `real_model_structural` — applies `InstructionDuplicationPass` to a real
    onnx-mlir `model.ll` and verifies `compareFloatValues` calls are inserted.
  - `real_model_end_to_end` — runs the baseline and duplicated models through
    `lli` and asserts outputs are identical (SKIP when `model.ll` absent).

#### Tooling
- `lint.sh` — unified C++ and Python lint runner; `--fix` auto-formats in-place.
- `.clang-tidy` — project-level tidy config with intentionally disabled checks
  documented.
- `.clang-format` — project-level format config (LLVM style, 2-space indent).
- `setup.cfg` — `flake8` and `flake8-bugbear` configuration for Python linting.

#### Documentation
- `architecture.md` — new developer reference covering pass pipeline, selector
  class hierarchy, hardware/software/ML fault modes, the runtime library, and
  the interface between compile-time and runtime layers.
- `docs/input_yaml_guide.md` — prose guide to writing `input.yaml` files,
  covering all keys, `CustomTensorOperator` ML targeting, and complete examples.
- `docs/tutorial_first_experiment.md` — end-to-end walkthrough of the
  `factorial` experiment including output file interpretation and outcome
  classification (masked / SDC / crash / hang).
- `docs/adding_a_test.md` — step-by-step guide for adding a regression test,
  covering program registration, test case structure, custom Python scripts,
  and the SKIP convention.
- `CODING_GUIDELINES.md` — expanded with sections on `override`, variable
  initialisation, container emptiness (`.empty()` over `.size() == 0`), and
  `cast<>` vs. `dyn_cast<>`.
- `CONTRIBUTING.md` — added `Adding a Test Case` section pointing to
  `docs/adding_a_test.md`.
- `docs/tutorial_ml_experiment.md` — new end-to-end walkthrough of an ML/ONNX
  fault injection experiment covering the full ONNX → LLVM IR compilation
  pipeline, `CustomTensorOperator` layer targeting, multi-fault injection
  options, per-layer profiling output, and `CompareLayerOutputs.py`.

---

### Changed

#### LLVM 20 API compatibility

| File | Change |
|------|--------|
| `llvm_passes/core/FaultInjectionPass.cpp` | 3 sites: `new AllocaInst/StoreInst/LoadInst` constructors updated to LLVM 17+ API |
| `llvm_passes/core/InstTracePass.cpp` | 6 sites: same; `getFirstNonPHIOrDbgOrLifetime()` now returns `BasicBlock::iterator` |
| `llvm_passes/core/Utils.cpp` | `M.getGlobalList().push_back()` → `new GlobalVariable(M, ...)` (removed in LLVM 17) |
| `bin/SoftwareFailureAutoScan.py` | `-load`/`-enable-new-pm=0` → `-load-pass-plugin`/`--passes=` (legacy PM removed in LLVM 17) |
| All selector `.cpp` files | `getNumArgOperands()` → `arg_size()` (removed in LLVM 15); `#include "llvm/Support/CFG.h"` → `"llvm/IR/CFG.h"` |
| `llvm_passes/instruction_duplication/InstructionDuplication.cpp` | `getNextNonDebugInstruction()` return type updated to `BasicBlock::iterator` |

#### Code quality (C++ — found by clang-tidy and code review)

| Category | Details |
|----------|---------|
| Bug fixes | Double-free in `Controller.cpp` destructor; file stream leak in `LLFIDotGraphPass.cpp`; unchecked `fopen` null in `GenLLFIIndexPass.cpp`; uninitialized `isChainDuplication` field |
| Null safety | `getCalledFunction()` null checks in `ProfilingPass.cpp`, `InstructionDuplication.cpp`, `CustomTensorOperatorInstSelector.cpp` |
| LLVM idioms | `dyn_cast<>` after `isa<>` → `cast<>` (asserting) across `Utils.cpp`, `_SoftwareFaultRegSelectors.cpp`, `_Timing_HighFrequentEventSelector.cpp`, `ProfilingPass.cpp`; `NULL` → `nullptr` throughout |
| Override safety | `virtual` on override methods → `override` keyword across all selector classes; `virtual ~Base() = default` added to abstract base classes |
| Style | `.empty()` over `.size() == 0`; `const auto&` in range-for; `strncpy`/`strncat` over unbounded `strcpy`/`strcat`; `cl::opt<T>::getValue()` to avoid slicing |
| Dead code | Removed unreachable `return false` after exhaustive if/else in `InstructionDuplication.cpp:runOnMainGraph()` |
| Copies | `for (auto insVector : arithInst)` → `for (const auto& insVector : ...)` to avoid copying inner vectors |

#### FIDL templates (`tools/FIDL/config/`)

All four templates (`TargetSingleTemplate.cpp`, `TargetMultiSourceTemplate.cpp`,
`TargetAllTemplate.cpp`, `NewInjectorTemplate.cpp`) updated:

- `virtual` on override methods → `override`
- `dyn_cast<>` after `isa<>` → `cast<>`
- `.size() == 0` → `.empty()`
- `std::string(getName())` → `.getName().str()`

All 37 FIDL-generated selectors were regenerated from the updated templates.

#### Code quality (Python — found by flake8/bugbear)

- `except:` → `except Exception:` throughout `bin/`, `tools/`, `test_suite/SCRIPTS/`
- Bare `open()` → `with open(...) as f:` in multiple scripts
- `subprocess(..., shell=True)` removed; replaced with list-form calls
- `yaml.load()` → `yaml.safe_load()` everywhere
- `exit()` → `sys.exit()` in scripts
- `%-format` strings → f-strings in new code

#### Docker

- `docker/Dockerfile` — LLVM source checkout updated from a pinned LLVM 15
  commit hash (`9778ec057cf4`) to the `llvmorg-20.1.0` tag; `pyyaml===5.4.1`
  corrected to `pyyaml==5.4.1` (non-standard triple-equals syntax).

#### Documentation updates

- `README.md` — restructured to eliminate overlap with `architecture.md`;
  added `docs/` section listing all user guides; added pointer to
  `architecture.md` for internal design.
- `caveats.txt` — LLVM version references updated 15 → 20; duplicate item
  number fixed.
- `llvm_passes/instruction_duplication/README.md` — `opt -always-inline`
  (legacy PM) → `opt --passes=always-inline` (new PM).
- `llvm_passes/instruction_duplication/shared_lib/build.sh` and
  `compile_shrd_lib.sh` — hardcoded `clang`/`clang++` → `LLVM_GXX_BIN_DIR`
  pattern, fixing builds on Ubuntu where apt installs `clang-20` only.
- `architecture.md` — corrected several inaccuracies found during code review:
  `preFunc` return type (`bool` not `int`) and parameter types (`unsigned`
  throughout); `injectFunc` register parameter types; `doProfiling` parameter
  type (`int` not `unsigned`); `printInstTracer` signature (second param is
  `char *opcode`, not `unsigned`; last param is `int`, not `long`);
  `lltfiMLLayer` parameter types (`int64_t`); removed non-existent `random`
  and `data_corruption` `fi_type` entries; corrected claim that
  `_FIDLSoftwareFaultInjectors.cpp` is generated/not-in-git (it is tracked);
  added missing `bin/` scripts, `LLFIDotGraphPass`, and memmove/memcpy
  intrinsic limitation; added design decisions for new-PM-only and separate
  `SEDPasses.so`.

---

### Pending before merge (human tasks)

- **H-2** — Human review of IRBuilder insertion-point correctness in
  `FaultInjectionPass.cpp` and `InstTracePass.cpp`.  The `AllocaInst` calls
  were migrated to `BasicBlock*` insertAtEnd form and `BasicBlock::iterator`
  form respectively; both compile and all 21 tests pass, but a developer
  familiar with the pass semantics should verify the insertion points are
  logically correct before merging to main.
- **H-3** — onnx-mlir real-model validation.  Requires installing onnx-mlir and
  running `sample_programs/ml_sample_programs/vision_models/mnist/compile.sh`
  to produce `model.ll`.  The two new `test_instruction_duplication.py` tests
  will then run instead of skipping.  Not a blocker — all other tests pass.

---

## [Previous] — LLVM 15 baseline (`master`)

The `master` branch represents LLTFI as it existed targeting LLVM 15, with
the following improvements over the original LLFI fork:

- ML fault injection support (TensorFlow, PyTorch via ONNX-MLIR)
- `CustomTensorOperator` instruction selector for layer-level ML targeting
- `InstructionDuplication` pass (`SEDPasses.so`) for soft-error detection
- Batch fault injection scripts (`batchInstrument.py`, `batchProfile.py`,
  `batchInjectfault.py`)
- FIDL software fault mode generator (37 modes across 6 failure classes)
- `SoftwareFailureAutoScan.py` for automatic software fault scanning
- Trace analysis tools (`tracediff.py`, `traceontograph.py`, `traceunion.py`,
  `tracetodot.py`)
- Makefile generation tool (`GenerateMakefile`)
- Initial `CODING_GUIDELINES.md` and `CONTRIBUTING.md`
- Migration plan document (`migration.md`) for the LLVM 15 → 20 upgrade
