# LLTFI Coding Guidelines

These guidelines apply to all hand-written source files in the project. C++ and C sources live under `llvm_passes/`, `runtime_lib/`, and `tools/`. Python sources live under `bin/`, `test_suite/SCRIPTS/`, `tools/`, and `setup`. FIDL-generated files under `llvm_passes/software_failures/` are auto-generated from templates in `tools/FIDL/config/` — apply changes there.

---

## C++ Standard

Target **C++17**. LLVM 20 requires it, and it provides range-based for loops, `nullptr`, `auto`, structured bindings, and other improvements used throughout the codebase.

---

## Naming Conventions

| Construct | Style | Example |
|-----------|-------|---------|
| Classes / structs | `PascalCase` | `FaultInjectionPass`, `FIInstSelector` |
| Functions / methods | `camelCase` | `getFIInsts()`, `isInstFITarget()` |
| Local variables | `snake_case` | `fi_inst`, `reg_pos_list` |
| Member variables | `snake_case` (no prefix) | `fi_rettype_funcname_map` |
| Constants / macros | `ALL_CAPS` | `DST_REG_POS`, `OPTION_LENGTH` |
| Namespaces | `lowercase` | `namespace llfi` |

---

## Null Pointers

Use `nullptr` in all C++ code. Never use `NULL` or `0` for pointer comparisons in C++ files.

```cpp
// Good
Value *fi_reg = nullptr;
if (mainfunc == nullptr) { ... }

// Bad
Value *fi_reg = NULL;
if (mainfunc == 0) { ... }
```

`NULL` remains acceptable in C files (`.c`).

---

## Include Guards

Every header file must have an include guard. Use `#ifndef` / `#define` / `#endif`. The guard name must match the filename (upper-cased, dots and slashes replaced with underscores).

```cpp
// FaultInjectionPass.h
#ifndef FAULT_INJECTION_PASS_H
#define FAULT_INJECTION_PASS_H

// ... contents ...

#endif // FAULT_INJECTION_PASS_H
```

Do **not** use `#pragma once` — it is not part of the C++ standard and is inconsistent with the rest of the codebase.

---

## Memory Management

Prefer stack allocation or LLVM's ownership model over manual `new`/`delete`.

```cpp
// Good — stack allocation
LegacyProfilingPass pass;
pass.runOnModule(M);

// Also good — LLVM IR objects are owned by the module
new AllocaInst(type, addrSpace, "", insertBefore);  // IR takes ownership

// Bad — manual heap allocation for short-lived objects
auto *obj = new LegacyProfilingPass();
obj->runOnModule(M);
delete obj;
```

When heap allocation is unavoidable outside of LLVM IR insertion, use `std::unique_ptr`.

---

## Return Statements

Every non-void function must have a return statement on all code paths. Missing returns in functions returning `bool` (e.g., `runOnModule`) are undefined behaviour and generate compiler warnings.

```cpp
// Good
virtual bool runOnModule(Module &M) {
  // ... do work ...
  return false;  // ModulePass: return true only if IR was modified
}
```

---

## LLVM Idioms

Use LLVM's type-checking utilities instead of C-style casts:

```cpp
// Good
if (isa<CallInst>(inst)) { ... }
CallInst *CI = dyn_cast<CallInst>(inst);
if (CI) { ... }

// Bad
if (dynamic_cast<CallInst*>(inst)) { ... }
CallInst *CI = (CallInst*)inst;
```

Use `llvm::errs()` for diagnostic output from passes, not `std::cerr`:

```cpp
// Good (in LLVM passes)
errs() << "ERROR: ...\n";

// Acceptable (in runtime C library)
fprintf(stderr, "ERROR: ...\n");
```

Use `StringRef` and `.str()` when converting LLVM names to `std::string`:

```cpp
std::string name = called_func->getName().str();  // Good
std::string name = called_func->getName();         // Bad: implicit conversion removed in LLVM 15
```

---

## Error Handling

- In LLVM passes: print a message with `errs()` and return early. Do not call `exit()`.
- In the runtime C library: `exit(1)` is acceptable for unrecoverable configuration errors (e.g., missing config file at startup), but not for conditions that can be recovered from.
- Always check the return value of `fopen` and similar system calls before using the result.

```cpp
// Good
FILE *f = fopen(filename, "r");
if (f == NULL) {
  fprintf(stderr, "ERROR: cannot open %s\n", filename);
  exit(1);
}
```

---

## Brace Style

Opening brace on the **same line** as the declaration (K&R / LLVM style):

```cpp
// Good
void FaultInjectionPass::insertInjectionFuncCall(...) {
  for (...) {
    if (...) {
    }
  }
}

// Bad (Allman style — don't use)
void FaultInjectionPass::insertInjectionFuncCall(...)
{
}
```

---

## Indentation

Use **2 spaces**. Do not use tabs. Most editors can be configured to convert tabs to spaces automatically.

---

## `using namespace` in Headers

Avoid `using namespace llvm;` in header files. It pollutes the namespace of every translation unit that includes the header. Existing headers use it for historical reasons; do not add it to new headers.

In `.cpp` files, `using namespace llvm;` is acceptable.

---

## Comments

- Use `//` for single-line and multi-line inline comments.
- Use the LLVM file banner (see `FaultInjectionPass.cpp`) for files that are part of the LLFI/LLTFI distribution.
- Remove contributor-specific annotations (e.g. `/*BEHROOZ: ...*/`, `//=== QINING @DATE ===`) before merging. Instead, document the *why* in a neutral comment or a commit message.
- Avoid commented-out code. Use version control instead.

---

## FIDL-Generated Files

Files under `llvm_passes/software_failures/` matching `_*_*Selector.cpp` are auto-generated by `tools/FIDL/FIDL-Algorithm.py` and are listed in `.gitignore`. Do **not** edit them directly — edit the corresponding template in `tools/FIDL/config/` and re-run FIDL:

```bash
python3 tools/FIDL/FIDL-Algorithm.py -a default
```

The `./setup` script runs this automatically during installation.

---

## Python Guidelines

### Python Version

All scripts must target **Python 3**. Use `#!/usr/bin/env python3` as the shebang. Do not use Python 2 syntax or shebangs.

### Naming Conventions

Follow **PEP 8** throughout:

| Construct | Style | Example |
|-----------|-------|---------|
| Functions / methods | `snake_case` | `check_input_yaml()`, `parse_args()` |
| Variables | `snake_case` | `option_list`, `base_dir` |
| Classes | `PascalCase` | `DiffBlock`, `FaultReport` |
| Constants (module-level) | `UPPER_CASE` | `DEFAULT_TIMEOUT` |

### Exception Handling

Never use a bare `except:` — it silently swallows `KeyboardInterrupt` and `SystemExit`. Use the most specific exception type available; fall back to `except Exception:` only when the exact type is genuinely unknown.

```python
# Good — specific
try:
    with open(path, 'r') as f:
        doc = yaml.safe_load(f)
except OSError:
    print("ERROR: cannot open", path)
    sys.exit(1)
except yaml.YAMLError:
    print("ERROR: invalid YAML in", path)
    sys.exit(1)

# Acceptable fallback when type is unclear
except Exception:
    ...

# Bad — catches everything including Ctrl-C
except:
    ...
```

### File Handling

Always use `with` statements (context managers) for file operations. This guarantees the file is closed even if an exception is raised.

```python
# Good
with open(filename, 'r') as f:
    data = f.read()

# Bad — file may not be closed on exception
f = open(filename, 'r')
data = f.read()
f.close()
```

### subprocess — Avoid `shell=True`

Do not pass `shell=True` to `subprocess` functions unless strictly necessary. Use a list of arguments instead. For output redirection (which requires a shell when using `>`), pass `stdout=open(output_file, 'w')` to subprocess directly.

```python
# Good
with open(output_file, 'w') as out:
    subprocess.call([script, arg1, arg2], stdout=out, stderr=log)

# Bad — shell injection risk, path-escaping burden
cmd = script + " " + arg1 + " > " + output_file
subprocess.call(cmd, shell=True)
```

### `sys.exit()` vs `exit()`

Use `sys.exit()` in scripts. `exit()` is intended for the interactive interpreter and is not guaranteed to be available in all environments.

```python
import sys
sys.exit(1)   # Good
exit(1)       # Bad
```

### String Formatting

Prefer **f-strings** for new code (Python 3.6+). Avoid `%`-formatting in new code.

```python
# Good
print(f"ERROR: No input.yaml in {srcpath}")

# Acceptable for existing code
print("ERROR: No input.yaml in {}".format(srcpath))

# Avoid in new code
print("ERROR: No input.yaml in %s" % srcpath)
```

### Imports

One import per line. Order: standard library → third-party → local. Separate each group with a blank line.

```python
import os
import sys

import yaml

from config import llvm_paths
```

### `yaml.safe_load()`

Always use `yaml.safe_load()`. Never use `yaml.load()` without an explicit `Loader` argument — it executes arbitrary Python and is a security risk.
