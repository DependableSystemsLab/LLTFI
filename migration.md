# LLTFI LLVM Upgrade Migration Guide

This document describes the work required to upgrade LLTFI from LLVM 15 to a
current LLVM release (17–20). Tasks are split between those that require a
human and those that Claude Code can handle autonomously once the build
environment is ready.

Migrated from: **LLVM 15.0** (`/usr/lib/llvm-15`)  
Current LLVM version: **LLVM 20.1** (`/usr/lib/llvm-20`)

---

## Status: further along than it appears

The most difficult part of a major LLVM upgrade — migrating from the legacy
pass manager (PM) to the new PM — is **already done for all core LLTFI
passes**. `RegisterPasses.cpp` exposes `llvmGetPassPluginInfo()`, every core
pass has a `PassInfoMixin` wrapper, and `instrument.py` already uses
`-load-pass-plugin`. What remains is a set of API-level fixups and one
incomplete pass migration.

---

## Human tasks (cannot be delegated to Claude Code)

### H-1 — Install LLVM 17+ and update the build configuration ✅ DONE
**Estimated time: 2–4 hours**

This is the only hard prerequisite. Claude Code cannot change what is installed
on the machine, so the compile-verify-fix loop that de-risks the code changes
cannot begin until this is done.

Steps:
1. Install a current LLVM release via the LLVM apt repository:
   ```bash
   wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh
   sudo ./llvm.sh 20   # or 17/18/19
   ```
2. Update the build configuration to point at the new installation:
   ```bash
   # Delete the old build root first
   rm -rf /home/karthik/Programs/LLTFI-build

   ./setup -LLFI_BUILD_ROOT /home/karthik/Programs/LLTFI-build \
           -LLVM_SRC_ROOT /home/karthik/Programs/llvm-project \
           -LLVM_DST_ROOT /usr/lib/llvm-20 \
           -LLVM_GXX_BIN_DIR /usr/lib/llvm-20/bin
   ```
3. Attempt an initial build (`cd LLTFI-build && make`) and record the errors.
   The output of this first build attempt is the input for Claude Code's work.

### H-2 — Review IRBuilder insertion-point correctness
**Estimated time: 2–3 hours**

The instruction-construction API fixes (task C-1 below) replace
`new AllocaInst/StoreInst/LoadInst` calls with IRBuilder equivalents. These
compile cleanly whether correct or not, but an incorrect insertion point places
an instruction in the wrong basic block or wrong position, producing
miscompiled IR. A human who understands the intended pass semantics should
review these diffs specifically before merging. The key files to scrutinise
are `FaultInjectionPass.cpp` and `InstTracePass.cpp`.

### H-3 — Validate InstructionDuplication against onnx-mlir generated IR
**Estimated time: 2–4 hours**

After the InstructionDuplication pass is migrated to the new PM (task C-3),
its behaviour must be verified on real onnx-mlir output, not just the
synthetic LLVM IR used in the unit tests. This requires onnx-mlir to be
available and the mnist sample model to be compiled. The unit tests in
`test_instruction_duplication.py` check structural correctness; this step
validates end-to-end numerical correctness of the duplicated arithmetic.

Steps:
1. Install onnx-mlir or set `ONNX_MLIR_BUILD` to point at an existing build.
2. Run `compile.sh` in `sample_programs/ml_sample_programs/vision_models/mnist/`.
3. Apply the InstructionDuplication pass to the resulting `model.ll` and run
   the model, comparing output against the non-duplicated baseline.

### H-4 — Final test sign-off
**Estimated time: 2–3 hours**

Run the full test suite against the new LLVM version and confirm no regressions:
```bash
cd /path/to/LLTFI-build/test_suite
python3 SCRIPTS/llfi_test --all        # expect 21/21
python3 SCRIPTS/llfi_test --all_ml     # expect all non-SKIP to pass
```
Any SKIP → FAIL regressions need human triage, as they may reflect genuine
behavioural changes in the new LLVM rather than API compilation errors.

---

## Claude Code tasks (can be done autonomously once LLVM 17+ is installed)

### C-1 — Replace deprecated instruction-construction API (9 sites) ✅ DONE
**Estimated time: 1 day**

LLVM 16/17 removed the `InsertBefore` Instruction-pointer parameter from
`AllocaInst`, `StoreInst`, and `LoadInst` constructors. All 9 affected sites
must be replaced with IRBuilder equivalents.

Affected files and sites:

| File | Sites | Pattern |
|------|-------|---------|
| `llvm_passes/core/FaultInjectionPass.cpp` | 3 | `new AllocaInst(...)`, `new StoreInst(...)`, `new LoadInst(...)` |
| `llvm_passes/core/InstTracePass.cpp` | 6 | `new AllocaInst(...)` ×3, `new StoreInst(...)` ×3 |

Example replacement:
```cpp
// Before (LLVM 15)
AllocaInst *tmploc = new AllocaInst(fitype, 0, "tmploc", entryblock);

// After (LLVM 17+)
IRBuilder<> builder(&entryblock->front());
AllocaInst *tmploc = builder.CreateAlloca(fitype, nullptr, "tmploc");
```

Note: `CallInst::Create(func, args, "", insertPoint)` with a raw `Instruction *`
as the last argument may also need updating to use a `BasicBlock::iterator`;
check each call site after the build reveals errors.

### C-2 — Fix iterator return-type changes ✅ DONE
**Estimated time: 2–3 hours**

Two methods changed their return type from `Instruction *` to
`BasicBlock::iterator` in LLVM 17:

- `getFirstNonPHIOrDbgOrLifetime()` — used in `InstTracePass.cpp:123`
- `getNextNonDebugInstruction()` — used in `Utils.cpp:125` and
  `InstructionDuplication.cpp:440–443`

At each call site, either dereference the iterator (`&*iter`) or update the
surrounding code to work with iterators directly.

### C-3 — Migrate InstructionDuplication to the new pass manager ✅ DONE
**Estimated time: 2–3 days**

`InstructionDuplication.cpp` is the only remaining pass still using the legacy
PM. It needs to be migrated to `PassInfoMixin` to match all other LLTFI passes.

Changes required:

1. **`InstructionDuplication.cpp`**: Change class base from `FunctionPass` to
   `PassInfoMixin<InstructionDuplicationPass>`. Rename `runOnFunction(Function
   &F)` to `run(Function &F, FunctionAnalysisManager &AM)` returning
   `PreservedAnalyses`. Remove `static char ID` and `RegisterPass<>`.
   The five existing `PassInfoMixin` passes in the codebase serve as templates.

2. **`RegisterPasses.cpp`**: Add a `registerPipelineParsingCallback` entry for
   `InstructionDuplicationPass`, following the existing pattern for the other
   passes. Decide whether `SEDPasses.so` should be merged into the main
   `llfi-passes.so` or kept separate; keeping it separate is simpler.

3. **`llvm_passes/instruction_duplication/CMakeLists.txt`**: Update to add the
   `llvmGetPassPluginInfo` export if `SEDPasses.so` stays separate, or adjust
   linking if merged.

4. **`shared_lib/build.sh` and `compile_shrd_lib.sh`**: Update `opt` invocation
   to remove `-load` / `--enable-new-pm=0` and use `-load-pass-plugin` with
   `--passes=InstructionDuplicationPass`.

5. **`README.md`** (in `instruction_duplication/`): Update example `opt`
   command.

6. **`test_suite/SCRIPTS/test_instruction_duplication.py`**: Update `_run_pass`
   to use `-load-pass-plugin` and `--passes=InstructionDuplicationPass` instead
   of `-load` / `--InstructionDuplicationPass` / `--enable-new-pm=0`.

### C-4 — Fix `Module::getGlobalList().push_back()` in `Utils.cpp` ✅ DONE
**Estimated time: 30 minutes**

`getGlobalList()` was removed in LLVM 17. The one call site in
`Utils.cpp:206` creates a `GlobalVariable` and appends it to the module.
Replace with the `GlobalVariable` constructor overload that takes a `Module *`
directly (which inserts the variable into the module automatically):

```cpp
// Before (LLVM 15)
nameStr = new GlobalVariable(name_c->getType(), true,
    GlobalVariable::InternalLinkage, name_c, gv_nameStr.c_str());
M.getGlobalList().push_back(nameStr);

// After (LLVM 17+)
nameStr = new GlobalVariable(M, name_c->getType(), true,
    GlobalVariable::InternalLinkage, name_c, gv_nameStr.c_str());
```

### C-5 — Fix `SoftwareFailureAutoScan.py` legacy PM flags ✅ DONE
**Estimated time: 30 minutes**

`bin/SoftwareFailureAutoScan.py:92` still uses `-load` and `-enable-new-pm=0`,
which were removed in LLVM 17. Update to match the style already used in
`instrument.py`:

```python
# Before
execlist = [optbin, "-load", llfipasses, "-genllfiindexpass",
            "-SoftwareFailureAutoScanPass", "-enable-new-pm=0"]

# After
execlist = [optbin, "-load-pass-plugin", llfipasses,
            "--passes=genllfiindexpass,SoftwareFailureAutoScanPass"]
```

### C-6 — Iterative build-fix loop ✅ DONE
**Estimated time: 1 week (wall clock; most of this is Claude Code running builds)**

After applying C-1 through C-5, run `make` and address any remaining compiler
errors introduced by LLVM 17–20 API changes not captured above. LLVM releases
between 17 and 20 introduce additional deprecations (e.g. changes to
`Value::use_iterator`, `DebugLoc` APIs, `MDNode` helpers) that may surface
depending on the exact target version. Claude Code can drive this loop:
read the error, identify the fix, apply it, rebuild.

---

## Recommended order of work

```
H-1  Install LLVM 17+ and attempt initial build
  └─> C-1  Fix instruction-construction API          (Claude, human reviews)
  └─> C-2  Fix iterator return-type changes          (Claude)
  └─> C-4  Fix getGlobalList                         (Claude)
  └─> C-5  Fix SoftwareFailureAutoScan.py            (Claude)
  └─> C-6  Iterative build-fix loop                  (Claude, human unblocks)
H-2  Review IRBuilder insertion-point diffs
  └─> C-3  Migrate InstructionDuplication pass       (Claude, human reviews)
H-3  Validate InstructionDuplication on onnx-mlir IR
H-4  Final test sign-off
```

---

## Effort summary

| Task | Owner | Status | Estimated time |
|------|-------|--------|---------------|
| H-1: Install LLVM 20 and update build config | Human | ✅ Done | 2–4 hours |
| H-2: Review IRBuilder insertion-point correctness | Human | Pending | 2–3 hours |
| H-3: Validate InstructionDuplication on onnx-mlir | Human | Pending | 2–4 hours |
| H-4: Final test sign-off | Human | Pending | 2–3 hours |
| **Total human time remaining** | | | **~6–10 hours** |
| C-1: Deprecated instruction-construction API | Claude Code | ✅ Done | — |
| C-2: Iterator return-type fixes | Claude Code | ✅ Done | — |
| C-3: InstructionDuplication new PM migration | Claude Code | ✅ Done | — |
| C-4: `getGlobalList` fix | Claude Code | ✅ Done | — |
| C-5: `SoftwareFailureAutoScan.py` flags | Claude Code | ✅ Done | — |
| C-6: Iterative build-fix loop | Claude Code | ✅ Done | — |
| **Total Claude Code time remaining** | | | **None — all done** |

Without Claude Code, a human developer would need approximately **2–3 weeks**
of active work. With Claude Code handling the mechanical fixes and the
build-fix loop, human involvement drops to roughly **1.5–2 days** of active
effort, with Claude Code running largely autonomously in between.

---

## What could go wrong

- **Subtle IRBuilder insertion-point bugs**: compile successfully but produce
  miscompiled IR. Mitigated by H-2 (human review) and the existing test suite.
- **New PM semantics differences**: the new PM runs passes in a different order
  and does not support inter-pass mutable state in the same way. The
  `Controller` singleton used by the FI selectors should be checked for
  thread-safety assumptions that the new PM may violate.
- **onnx-mlir compatibility**: onnx-mlir targets a specific LLVM version
  internally. If the onnx-mlir build and the LLTFI build target different LLVM
  versions, llvm-link may refuse to link the bitcode. This is an environment
  concern, not a code concern, but it could block H-3.
