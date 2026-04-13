# Adding a Test Case to the LLTFI Test Suite

This document explains the structure of the regression test suite and how to
add a new test.  Read it when CONTRIBUTING.md says "new functionality has a
corresponding test case" and you are not sure where to start.

---

## Overview of the test harness

The test suite lives in `test_suite/` (source tree) and is copied into
`<LLFI_BUILD_ROOT>/test_suite/` during installation.  The driver script is:

```
test_suite/SCRIPTS/llfi_test
```

Tests are split into categories invoked with flags such as `--all_hardware_faults`,
`--all_software_faults`, `--all_trace_tools_tests`, etc.  The category for most
new fault injection tests is either **HardwareFaults** or **SoftwareFaults**.

---

## Directory layout

```
test_suite/
  PROGRAMS/              Compiled IR files (.ll / .bc) and input data
    factorial/           One sub-directory per program
      factorial.c
      Makefile
    mcf/
      ...
    Makefile             Builds all PROGRAMS/* in one pass
    Makefile.common      Shared clang flags, included by every program Makefile

  HardwareFaults/        One sub-directory per hardware-fault test case
    random/
      input.yaml         Controls instrumentation and injection for this test
      (MCF.ll, inp.in)   Deployed here at test time from PROGRAMS/
    funcname/
      ...

  SoftwareFaults/        Same structure for software-fault test cases
    wrong_api/
      input.yaml
      ...

  test_suite.yaml        Master registry — lists programs, their files, and
                         which test case uses which program

  SCRIPTS/
    llfi_test            Top-level driver
    build_prog.py        Builds programs in PROGRAMS/
    deploy_prog.py       Copies program files into test case directories
    inject_prog.py       Runs instrument → profile → injectfault for each case
    check_injection.py   Verifies the llfi/ output directory is well-formed
    test_trace_tools.py  Trace-tool specific tests
    test_fidl_generation.py  FIDL selector count tests
    test_instruction_duplication.py  SID pass tests
    test_ml_models.py    ML fault injection tests
    test_ml_tools.py     ML tool unit tests
    test_software_failure_autoscan.py  Auto-scan tests
```

---

## How a fault injection test runs

When `llfi_test --all_hardware_faults` runs, it calls these scripts in order:

1. **`build_prog.py`** — builds all programs listed in `test_suite.yaml` under
   `PROGRAMS:` by running `make` in each program's directory.

2. **`deploy_prog.py`** — copies the compiled files (e.g. `MCF.ll`, `inp.in`)
   from `PROGRAMS/mcf/` into each test case directory (e.g. `HardwareFaults/random/`)
   that uses that program.

3. **`inject_prog.py`** — for each test case directory:
   - `cd` into the directory
   - runs `instrument` on the `.ll` file
   - runs `profile`
   - runs `injectfault`
   - logs stdout/stderr to `llfi.test.log.{instrument,profile,injectFault}.txt`

4. **`check_injection.py`** — verifies that each test case directory now contains
   a well-formed `llfi/` tree (subdirectories present, at least one stat file).
   Reports PASS or FAIL.

---

## Adding a HardwareFaults or SoftwareFaults test

### Step 1 — Decide which program to use

Check whether an existing program in `PROGRAMS/` suits the new test.  Reusing
a program is strongly preferred.  If you need a new program, follow Step 1a.

**Step 1a (if a new program is needed) — Add a program**

Create a subdirectory under `test_suite/PROGRAMS/`:

```
test_suite/PROGRAMS/myprogram/
  myprogram.c          (or myprogram.ll if you hand-write IR)
  Makefile
  myinput.txt          (input data files, if needed)
```

The `Makefile` must produce a `.ll` file.  Use an existing program's Makefile
as a template:

```makefile
TARGET = myprogram
include ../Makefile.common

SRC_FILES = $(wildcard *.c)
OBJECTS   = $(SRC_FILES:.c=.bc)
LL_FILE   = $(TARGET).ll

default: all
all: $(LL_FILE)

%.ll: %.bc
	$(LLVMDIS) $< -o $@

%.bc: %.c
	$(LLVMGCC) $(COMPILE_FLAGS) $< -c -o $@

clean:
	$(RM) -f *.bc *.ll
```

Then register the program and its files in `test_suite.yaml`:

```yaml
PROGRAMS:
    myprogram:
        - myprogram.ll
        - myinput.txt   # list every file that needs to be deployed
```

And add the program input under `INPUTS:`:

```yaml
INPUTS:
    myprogram: myinput.txt   # passed as argv to the program; omit if none
```

### Step 2 — Create the test case directory

```bash
mkdir test_suite/HardwareFaults/my_new_test
```

(Use `SoftwareFaults/` for software fault tests.)

Write an `input.yaml` in that directory.  Example for a hardware fault test:

```yaml
defaultTimeOut: 500

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
        numOfRuns: 5
        fi_type: bitflip
```

See `docs/input_yaml_guide.md` for all available keys.

### Step 3 — Register the test case in `test_suite.yaml`

Add an entry under `HardwareFaults:` (or `SoftwareFaults:`):

```yaml
HardwareFaults:
    my_new_test: myprogram    # value is the PROGRAMS key
```

The value tells `deploy_prog.py` which program's files to copy into the test
case directory before running injection.

### Step 4 — Run the test locally

From the **build** directory:

```bash
cd <LLFI_BUILD_ROOT>/test_suite

# Rebuild programs (only needed once, or after source changes)
python3 SCRIPTS/build_prog.py

# Run only the new test
python3 SCRIPTS/llfi_test --test_cases HardwareFaults/my_new_test
```

A PASS result means the harness found a well-formed `llfi/` directory after
injection.  Check that the stat files are non-empty and that the output makes
sense for your test case.

### Step 5 — Verify it does not break the full suite

```bash
python3 SCRIPTS/llfi_test --all
```

Expected: 21/21 (or 22/22 if your test adds to the count) PASS.

---

## Adding a more specific test (custom Python script)

For tests that check something beyond "did injection produce a well-formed
`llfi/` directory" — e.g., counting the number of injected faults, verifying
a specific stat value, or testing a tool that is not part of the injection
pipeline — write a standalone Python test script in `test_suite/SCRIPTS/`.

Look at `test_trace_tools.py` or `test_fidl_generation.py` for examples of
the pattern:

```python
#!/usr/bin/env python3
"""
One-line description.

Usage: python3 SCRIPTS/test_my_feature.py
"""

import os
import sys

PASS = "PASS"
FAIL = "FAIL"
SKIP = "SKIP"

def test_something():
    # ... perform the check ...
    if condition:
        return PASS, "explanation"
    else:
        return FAIL, "what went wrong"

def main():
    results = []
    results.append(("my_check_name", test_something()))

    passed = failed = skipped = 0
    for name, (status, msg) in results:
        print(f"{name}: {status}" + (f" — {msg}" if msg else ""))
        if status == PASS:
            passed += 1
        elif status == FAIL:
            failed += 1
        else:
            skipped += 1

    print(f"\n{passed} PASS, {failed} FAIL, {skipped} SKIP")
    sys.exit(0 if failed == 0 else 1)

if __name__ == "__main__":
    main()
```

### SKIP convention

Return `SKIP` (not `FAIL`) when a test requires an optional dependency that
is not installed.  This keeps the suite green on machines that do not have
TensorFlow, onnx-mlir, etc.  Always print a short message explaining what is
missing:

```python
try:
    import onnx
except ImportError:
    return SKIP, "onnx not installed — pip install onnx"
```

### Registering a custom script in `llfi_test`

If the script should run as part of `--all` or a specific `--all_*` flag, add
a call to it inside `llfi_test`.  Search for an existing call (e.g. to
`test_trace_tools.py`) to see the pattern:

```python
# In llfi_test, inside the relevant branch:
r = subprocess.call([sys.executable,
                     os.path.join(script_dir, "test_my_feature.py")])
```

If it is an optional test (ML, ONNX), add it under the `--all_ml` branch.

---

## Checklist before submitting

- [ ] New program (if any) has a `Makefile` that produces a `.ll` file
- [ ] Program and its input files are registered in `test_suite.yaml`
- [ ] Test case directory exists with a valid `input.yaml`
- [ ] Test case is registered under the correct category in `test_suite.yaml`
- [ ] `python3 SCRIPTS/llfi_test --test_cases <category/name>` reports PASS
- [ ] `python3 SCRIPTS/llfi_test --all` still reports 21/21 (or N/N) PASS
- [ ] FIDL-generated `_*_*Selector.cpp` files are **not** staged for commit
