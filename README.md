LLTFI
=====
LLTFI (Low-Level Tensor Fault Injector) is a unified SWiFI tool that supports
fault injection of both C/C++ programs and ML applications written using
high-level frameworks such as TensorFlow and PyTorch.  Faults are injected at
the LLVM IR level, giving precise control over which instructions and registers
are targeted.

LLTFI is built on top of [LLFI](https://github.com/DependableSystemsLab/LLFI)
and is fully backward compatible with it.

For a detailed description of the internal architecture — pass pipeline,
selector class hierarchy, hardware/software/ML fault modes, runtime library,
and the interface between the compile-time and runtime layers — see
**[architecture.md](architecture.md)**.

Please refer to the following
[paper](https://blogs.ubc.ca/dependablesystemslab/2021/08/31/wip-lltfi-low-level-tensor-fault-injector/)
for background on LLTFI.


Repository Layout
-----------------

```
llvm_passes/          LLVM pass plugin (llfi-passes.so) — compile-time only
  core/                 Pass infrastructure and selector framework
  hardware_failures/    Built-in hardware fault instruction selectors
  software_failures/    Software fault selectors (hand-written + FIDL-generated)
  instruction_duplication/  SID pass for ML soft-error detection (SEDPasses.so)
runtime_lib/          C/C++ runtime library linked into instrumented binaries
bin/                  Python driver scripts: instrument.py, profile.py, injectfault.py
tools/                Trace analysis, FIDL code generator, ML utilities
  FIDL/                 Software fault mode generator (see architecture.md §4)
  GenerateMakefile/     Test harness Makefile generator
docs/                 tutorial_first_experiment.md — end-to-end C/C++ walkthrough and output guide
                      tutorial_ml_experiment.md — end-to-end ML/ONNX walkthrough (layer targeting, multi-fault)
                      adding_a_test.md — how to add a regression test case
                      input_yaml_guide.md — user guide for writing input.yaml
                      input_masterlist.yaml — full reference schema for input.yaml
test_suite/           Regression tests
sample_programs/      Example C/C++ and ML programs with input.yaml files
architecture.md       Internal architecture reference for developers
CODING_GUIDELINES.md  C++ and Python style rules
CONTRIBUTING.md       How to set up a dev environment and submit changes
migration.md          LLVM 15 → 20 upgrade log
```


Dependencies
------------

1. 64-bit Linux (Ubuntu 20.04 or later) or macOS
2. CMake ≥ 3.15
3. Python 3
4. Python YAML library (PyYAML ≥ 5.4.1)
5. Ninja ≥ 1.10.2
6. **Clang and LLVM 20.x**

   Easiest install on Ubuntu via the LLVM apt repository:
   ```bash
   wget https://apt.llvm.org/llvm.sh && chmod +x llvm.sh
   sudo ./llvm.sh 20
   ```

   To build LLVM from source (required if you also need MLIR for onnx-mlir):
   ```bash
   git clone https://github.com/llvm/llvm-project.git
   cd llvm-project && git checkout llvmorg-20.1.0 && cd ..
   mkdir llvm-project/build && cd llvm-project/build
   cmake -G Ninja ../llvm \
       -DLLVM_ENABLE_PROJECTS="clang;mlir" \
       -DLLVM_BUILD_TESTS=ON \
       -DLLVM_TARGETS_TO_BUILD="host" \
       -DLLVM_ENABLE_ASSERTIONS=ON \
       -DLLVM_ENABLE_RTTI=ON
   cmake --build . --target clang check-mlir mlir-translate opt llc lli \
       llvm-dis llvm-link -j$(nproc)
   ```

7. **For ML programs** (all optional; tests skip gracefully when absent):

   | Dependency | Install |
   |------------|---------|
   | TensorFlow ≥ 2.0 | `pip install tensorflow` |
   | tensorflow-onnx | `pip install tf2onnx` |
   | PyTorch | `pip install torch` |
   | ONNX | `pip install onnx` |
   | pygraphviz, pydot | `pip install pygraphviz pydot` |
   | libprotoc ≥ 3.11 | build from source (see below) |
   | [ONNX-MLIR](https://github.com/DependableSystemsLab/onnx-mlir-lltfi) (LLTFI branch) | see below |

   **libprotoc:**
   ```bash
   curl -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.17.2/protobuf-all-3.17.2.zip
   unzip protobuf-all-3.17.2.zip && cd protobuf-3.17.2
   ./configure && make -j$(nproc) && sudo make install && sudo ldconfig
   ```

   **ONNX-MLIR** (LLTFI branch, requires an MLIR-enabled LLVM build):
   ```bash
   git clone --recursive https://github.com/DependableSystemsLab/onnx-mlir-lltfi.git
   mv onnx-mlir-lltfi onnx-mlir && cd onnx-mlir && git checkout LLTFI && cd ..
   MLIR_DIR=$(pwd)/llvm-project/build/lib/cmake/mlir
   mkdir onnx-mlir/build && cd onnx-mlir/build
   cmake -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/c++ -DMLIR_DIR=${MLIR_DIR} ..
   cmake --build . && ninja install
   ```

8. GraphViz (for dependency graph visualisation)


Installation
------------

Run `./setup --help` for a full option list.

```
./setup -LLFI_BUILD_ROOT <build-dir> \
        -LLVM_SRC_ROOT   <llvm-project-dir> \
        -LLVM_DST_ROOT   <llvm-install-or-build-dir>
```

On Ubuntu where LLVM is installed via apt, `clang` is only available as
`clang-20`. Pass `-LLVM_GXX_BIN_DIR` explicitly:

```bash
./setup -LLFI_BUILD_ROOT /path/to/LLTFI-build \
        -LLVM_SRC_ROOT   /path/to/llvm-project \
        -LLVM_DST_ROOT   /usr/lib/llvm-20 \
        -LLVM_GXX_BIN_DIR /usr/lib/llvm-20/bin
```

The build root must not already exist. Delete it first when rebuilding from
scratch. To rebuild after source changes without re-running setup:

```bash
cd /path/to/LLTFI-build && make
```


Docker
------

`docker/Dockerfile` builds and runs LLTFI in a container.  Copy the Dockerfile
outside the repository, then:

```bash
docker build --tag lltfi .
docker run -it lltfi
```

See [docker/README.md](docker/README.md) for details.


Running Tests
-------------

Tests must be run from the **build** directory.  Running all regression tests
after installation is strongly recommended.

```bash
cd <LLFI_BUILD_ROOT>/test_suite

python3 SCRIPTS/llfi_test --all                    # 21 core tests (expected: 21/21 PASS)
python3 SCRIPTS/llfi_test --all_hardware_faults    # hardware fault injection (8 tests)
python3 SCRIPTS/llfi_test --all_software_faults    # software fault injection (5 tests)
python3 SCRIPTS/llfi_test --all_trace_tools_tests  # trace analysis tools (3 tests)
python3 SCRIPTS/llfi_test --all_makefile_generation # Makefile generation (2 tests)
python3 SCRIPTS/llfi_test --all_fidl               # FIDL generator (3 tests)
```

Error messages during fault injection runs are normal and expected.

#### ML / ONNX tests (optional dependencies)

```bash
python3 SCRIPTS/llfi_test --all_ml
```

Tests that require missing dependencies are reported as **SKIP** (not FAIL) and
excluded from the pass/fail count.

| Group | Requirements |
|-------|-------------|
| `SoftwareFailureAutoScan` | LLTFI build only |
| ML tool unit tests | `pip install onnx pygraphviz pydot` |
| Instruction duplication (synthetic IR) | LLTFI build only |
| Instruction duplication (real model IR) | `model.ll` from `sample_programs/.../mnist/compile.sh` |
| TensorFlow → ONNX | `pip install tensorflow tf2onnx onnx` |
| PyTorch → ONNX | `pip install torch onnx` |
| ONNX → LLVM IR | onnx-mlir binary (set `ONNX_MLIR_BUILD`) |
| Fault injection (ML) | LLTFI build + `model.ll` |

For ML fault injection tests, `model.ll` must be pre-built by running
`compile.sh` in `sample_programs/ml_sample_programs/vision_models/mnist/`
(requires onnx-mlir).


Running a Sample Program
------------------------

Programs in `sample_programs/` already contain a valid `input.yaml`.

Example — `factorial`:

1. Copy the directory to your working location:
   ```bash
   cp -r sample_programs/cpp_sample_programs/factorial/ /tmp/factorial
   cd /tmp/factorial
   ```
2. Set environment variables:
   ```bash
   export LLFI_BUILD_ROOT=/path/to/LLTFI-build
   export PATH=/path/to/llvm/bin:$PATH
   ```
3. Compile and run:
   ```bash
   bash compileAndRun.sh factorial 6
   ```

Output from LLFI is written to the `llfi/` directory.  See
[architecture.md §5.3](architecture.md) for a description of the output files.


Results
-------

After fault injection, output is in the `llfi/` directory inside your program
folder.  For a full description of each file see
[architecture.md — Interface Between the Two Layers](architecture.md).

| Directory | Contents |
|-----------|----------|
| `std_output/` | Piped stdout from each run |
| `llfi_stat_output/` | Fault injection statistics, profiling data, trace files |
| `error_output/` | Failure reports (crashes, hangs, SDCs) |
| `baseline/` | Golden output and profiling trace |
| `prog_output/` | Disk output from faulty runs |


Reproducing ISSRE'23 Experiments
---------------------------------

See the [ISSRE'23 AE branch README](https://github.com/DependableSystemsLab/LLTFI/blob/ISSRE23_AE/README.md).


References
----------

* [LLFI Paper](http://blogs.ubc.ca/karthik/2013/02/15/llfi-an-intermediate-code-level-fault-injector-for-soft-computing-applications/)
* [LLFI Wiki](https://github.com/DependableSystemsLab/LLFI/wiki)
* [LLTFI Wiki](https://github.com/DependableSystemsLab/LLTFI/wiki)
* Udit Kumar Agarwal, Abraham Chan, Karthik Pattabiraman. *LLTFI: Framework agnostic fault injection for machine learning applications.* ISSRE 2022. [PDF](https://www.dropbox.com/s/lgr3ed75sy0fq2p/issre22-camera-ready.pdf?dl=0)
* Udit Kumar Agarwal, Abraham Chan, Karthik Pattabiraman. *Resilience Assessment of Large Language Models under Transient Hardware Faults.* ISSRE 2023. [PDF](https://www.dropbox.com/scl/fi/mv6yehk0lctcz3l4efy0k/ISSRE23_Udit.pdf?rlkey=dzwbxk7js29pqjwirjj25ik8q&dl=0)


Citations
---------

```bibtex
@article{Agarwal22LLTFI,
  title   = {LLTFI: Framework agnostic fault injection for machine learning
             applications (Tools and Artifacts Track)},
  author  = {Agarwal, Udit and Chan, Abraham and Pattabiraman, Karthik},
  journal = {International Symposium on Software Reliability Engineering (ISSRE)},
  year    = {2022},
  publisher = {IEEE}
}
```

---

Read *caveats.txt* for known limitations and gotchas.

Read *CODING_GUIDELINES.md* for C++, C, and Python coding conventions.

Read *architecture.md* for a detailed description of the internal architecture.
