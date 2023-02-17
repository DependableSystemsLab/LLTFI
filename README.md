HDFIT + LLTFI Information
=====

This version of LLTFI is customized in order to operate in conjunction with the [HDFIT](https://intellabs.github.io/HDFIT/) fault injection framework, on a variety of HPC workloads. Please refer to the README below for instructions on how to build LLTFI, as well as to the HDFIT [documentation](https://github.com/IntelLabs/HDFIT.ScriptsHPC).

Please note that a series of HDFIT-specific build options were added to LLTFI: these are documented, and can be toggled, in LLTFI's global [CMakeLists.txt](CMakeLists.txt) file.

LLTFI
=====
LLTFI (Low Level Tensor Fault Injector) is a unified SWiFI (Software-implemented fault injection) tool that supports fault injection of both C/C++ programs and ML applications written using high-level frameworks such as TensorFlow and PyTorch.

As machine learning (ML) has become more prevalent across many critical domains, so has the need to understand ML system resilience. While there are many ML fault injectors at the application level, there has been little work enabling fault injection of ML applications at a lower level. **LLTFI** is a tool that allows users to run fault injection experiments on C/C++, TensorFlow and PyTorch applications at a lower level (at the LLVM IR level). Please refer to the following [paper](https://blogs.ubc.ca/dependablesystemslab/2021/08/31/wip-lltfi-low-level-tensor-fault-injector/) for more information about LLTFI.

LLTFI is built on top of [LLFI](https://github.com/DependableSystemsLab/LLFI) and is fully backwards compatible with it. 

### LLFI ###
**LLFI** is an LLVM based fault injection tool, that injects faults into the LLVM IR of the application source code.  The faults can be injected into specific program points, and the effect can be easily tracked back to the source code.  LLFI is typically used to map fault characteristics back to source code, and hence understand source level or program characteristics for various kinds of fault outcomes. Detailed documentation about LLFI can be found at: https://github.com/DependableSystemsLab/LLFI/wiki. Because LLTFI is designed to be backwards compatible with LLFI, the basic setup instructions for LLTFI are similar to those of LLFI. But, there are additional steps and dependencies for running ML programs. 

LLTFI Workflow:
-------------------------
High-level ML models need to be lowered to intermediate representation (IR) for fault injection. LLTFI provides a single script that converts ML models into LLVM IR, using several publicly available tools and performs fault injection.
LLTFI first lowers ML models to **MLIR** (Multi-Level Intermediate Representation) using ONNX-MLIR before converting to LLVM IR. Reasons for choosing MLIR being MLIR's ability to better preserve the semantics of ML models, its integration with LLVM, testability and easier extensibility. 

#### Workflow Diagram of LLTFI: ####

![Alt text](images/workflow.png?raw=true "Workflow Diagram of LLTFI")

- LLTFI first converts all ML models to the ONNX format. ONNX’s open exchange format allows LLTFI to
support both TensorFlow and PyTorch. 
- Then, the ONNX file is converted into MLIR through ONNX-MLIR. 
- Finally, we convert MLIR into LLVM IR, using the mlir-translate tool in LLVM 15.0. 

**LLTFI** can now inject faults into the LLVM IR, alike lowered C/C++ programs. 

The LLFI tool was originally written for LLVM 3.4. While developing LLTFI, the entire LLFI tool was upgraded to LLVM 15.0 because LLVM 3.4 has no support for MLIR.
This upgrade also ensured that LLTFI is compatible all of the newest C/C++ features, and LLVM optimization passes


Auto-Installer
--------------
If you wish to build LLTFI and its dependencies via the auto-installer(installer/InstallLLTFI.py), you *do not need* to clone the LLTFI git repository. Simply download the installer script by itself, and it will fetch the latest version of the git repository for you. To run the script, simply copy it into the directory where you would like to build the LLTFI and, from the command line, run `python3 InstallLLTFI.py`.
  
Dependencies:
  1. 64 Bit Machine (preferably with GPU for faster training of ML programs) 
  2. 64 bit Linux (Ubuntu 20.04) or OS X
  3. CMake (minimum v3.15)
  4. Python 3 and above
  5. Ninja >= 1.10.2
  6. Internet Connection

Usage:
  1. Copy the InstallLLTFI.py script to where you want to build the LLTFI. Run "python3 InstallLLTFI.py -h" to see all running options/guidelines
  2. Run "python3 InstallLLTFI.py"


Manual Installation
-------------------

In this method, the developer has more control over the location of the LLVM build that the LLTFI requires. If you already have LLVM built, you could use that build.

**Dependencies:**
  
  1. 64 Bit Machine (preferably with GPU for faster training of ML programs) 
  2. 64 bit Linux (Ubuntu 20.04) or OS X
  3. CMake (minimum v3.15)
  4. Python 3 and above
  5. Python YAML library (PyYAML v5.4.1)
  6. Ninja >= 1.10.2
  7. libprotoc >= 3.11.0
  8. Clang v15.0 (commit: 9778ec057cf4)
  9. LLVM v15.0 (commit: 9778ec057cf4) ([Reference](http://llvm.org/docs/CMake.html)).
		LLVM 15.0 takes a long time to completely build. Following is a shortcut to checking out the required LLVM commit, and building only the necessary LLVM targets.
		```
		git clone https://github.com/llvm/llvm-project.git
		
		# Check out a specific branch that is known to work with the required version of ONNX MLIR.
		cd llvm-project && git checkout 9778ec057cf4 && cd ..
			
		mkdir llvm-project/build
		cd llvm-project/build
		
		cmake -G Ninja ../llvm \
			-DLLVM_ENABLE_PROJECTS="clang;mlir" \
			-DLLVM_BUILD_TESTS=ON \
			-DLLVM_TARGETS_TO_BUILD="host" \
			-DLLVM_ENABLE_ASSERTIONS=ON \
			-DLLVM_ENABLE_RTTI=ON

		cmake --build . --target clang check-mlir mlir-translate opt llc lli llvm-dis llvm-link -j 2

		ninja install -j 2
		```
  10. For executing ML programs, following additional dependencies have to be installed:
		1. TensorFlow framework (v2.0 or greater)
		2. numpy package (part of TensorFlow)
		3. [tensorflow-onnx](https://github.com/onnx/tensorflow-onnx): 
		    Installation with pip is sufficient
		    ```
		    pip install tf2onnx
		    ```
		4. libprotoc
			```
			curl -OL https://github.com/protocolbuffers/protobuf/releases/download/v3.17.2/protobuf-all-3.17.2.zip
			unzip protobuf-all-3.17.2.zip
			cd protobuf-3.17.2
			
			./configure
			make -j 2
			make check
			sudo make install
			sudo ldconfig # refresh shared library cache.
			```
		5. [ONNX-MLIR](https://github.com/onnx/onnx-mlir)

		    Additional changes made in the ONNX-MLIR code are present in: https://github.com/DependableSystemsLab/onnx-mlir-lltfi. Clone this repo and checkout the `LLTFI` branch. The MLIR_DIR cmake variable must be set before building onnx-mlir. It should point to the mlir cmake module inside an llvm-project build or install directory (e.g., llvm-project/build/lib/cmake/mlir).
            ```
		    MLIR_DIR=$(pwd)/llvm-project/build/lib/cmake/mlir
		    ```

		    Onnx-mlir branch ``` LLTFI ``` has to be built and installed. 
			```
			git clone --recursive https://github.com/DependableSystemsLab/onnx-mlir-lltfi.git
			mv onnx-mlir-lltfi onnx-mlir && cd onnx-mlir
			git checkout LLTFI
			cd ..
	
			mkdir onnx-mlir/build && cd onnx-mlir/build
			cmake -G Ninja \
				-DCMAKE_CXX_COMPILER=/usr/bin/c++ \
				-DMLIR_DIR=${MLIR_DIR} \
				.. 
				
			cmake --build .
				
			# Run lit tests:
			export LIT_OPTS=-v
			cmake --build . --target check-onnx-lit
			
			ninja install
			```
  10. GraphViz package (for visualizing error propagation)



<!--
GUI Dependencies:
  1. JDK7/JDK8 with JavaFX
  2. tcsh shell
-->

### Building LLTFI: ###
  
  Run `./setup --help` for build instructions.
```
  $ ./setup --help

  Usage: setup OPTIONS
  List of options:
  -LLVM_DST_ROOT <LLVM CMake build root dir>:
      Make sure you build LLVM with CMake and pass build root directory here
  -LLVM_SRC_ROOT <LLVM source root dir>
  -LLFI_BUILD_ROOT <path where you want to build LLFI>
  -LLVM_GXX_BIN_DIR <llvm-gcc/g++'s parent dir> (optional):
      You don't need to set it if it is in system path
  -JAVA_HOME_DIR <java home directory for oracle jdk 7 or higher> (optional):
    You don't need to set it if your default jdk is oracle jdk 7 or higher and in system path


  --help(-h): show help information
  --runTests: Add this option if you want to run all regression tests after building LLFI.
```

  Below is the command to build LLTFI(without GUI) if `clang` is already in $PATH:
```
./setup -LLFI_BUILD_ROOT $BUILD/LLFI -LLVM_SRC_ROOT $SRC/llvm-15.0 -LLVM_DST_ROOT $BUILD/llvm-15.0
```

Details about running the Web GUI for LLTFI can be found [here](web-app/README.MD) 

### Building LLTFI using Docker: ###

`docker/Dockerfile` can be used to build and run LLTFI in a docker container. You can modify the Dockerfile according to your system and project requirements. More details can be found [here](docker/README.md)

Steps to build:
1. **Creating a docker image from the Dockerfile:** Copy the Dockerfile to a directory of your choice outside this repository. To create an image, run the command `docker build --tag imageName .` in the terminal.
2. **Starting a docker container:** Once the above step is completed, a docker container can be started using the command `docker run -it imageName`


### Running tests: ###
Running all regression tests after installation is highly recommended. Note that you may encounter some error messages during the fault injection stage. This is normal. Once all tests have completed and they all passed, LLFI is correctly installed.

For complete test of whole of LLFI, please use LLFI test suite and refer to wiki page: [Test suite for regression test](https://github.com/DependableSystemsLab/LLFI/wiki/Test-Suite-for-Regression-Test) for details.

<!--
VirtualBox Image
-----------------

If you want to quickly try out LLFI, an Ubuntu image with LLFI and its dependencies pre-installed 
is available [here](https://drive.google.com/file/d/0B5inNk8m9EfeM096ejdfX2pTTUU/view?usp=sharing) (2.60GB). This image is built with VirtualBox v4.3.26, with Ubuntu 14.04.2 LTS, LLVM v3.4, CMake v3.4 and the current master branch version of LLFI (as of Sep 16th, 2015).

user: `llfi`  
password: `root`

`<LLFI_SRC_ROOT>` is located under `~/Desktop/llfisrc/`.  
`<LLFI_BUILD_ROOT>` is located under `~/Desktop/llfi/`.  
`<LLVM_SRC_ROOT>` is located under `~/Desktop/llvmsrc/`.  
`<LLVM_DST_ROOT>` is located under `~/Desktop/llvm/`.  
`<LLVM_GXX_BIN_DIR >` is located under `~/Desktop/llvm/bin/`.  

Sample tests can be found under `~/Desktop/test/`.

To run it, open VirtualBox, select `File->Import Appliance...` and navigate to the `.ova` file.
-->

### Running Sample Programs ###

You can use test programs in the directory `sample_programs/` or `test_suite/PROGRAMS/` to test LLFI. Programs in the `sample_programs` directory already contains a valid `input.yaml` file.

Example program: `factorial`:
  1. Copy the `sample_programs/factorial/` directory to your project directory. 
  2. Set LLFI_BUILD_ROOT environment variable e.g., export LLFI_BUILD_ROOT=/path/to/LLFI/installation
  3. Call the ./compileAndRun.sh script with the first argument as factorial, and the second argument as the number to compute the factorial of (e.g., 6)


<!--
####GUI
If you have used `./setup` to install LLFI, you need to set new environment variables for tcsh shell before running the GUI for the first time. Open `~/.tcshrc` using your favourite text editor and add `setenv llfibuild <LLFI_BUILD_ROOT>/` and `setenv zgrviewer <LLFI_BUILD_ROOT>/tools/zgrviewer/` to it. [OPTIONAL] Create an environment variable "COMPARE" with the path of the SDC check script.

Execute `<LLFI_BUILD_ROOT>/bin/llfi-gui` to start the **GUI**. The outputs will be saved in the directory where you have executed the command.

####Web GUI Development Environment Setup
Dependencies:
Nodejs
webpack   

Steps to set up the development environment:   
1: Download this project from Git   
2: Download NodeJs   
3: Install libraries: Go to the web-app directory and run "npm install"   
4: Install Webpack: In the same directory as step 3, run "sudo npm install -g webpack"   
5: Configurate the LLFI root path for the server:   
The default bevaiour of the program use environment variable $llfibuild as the path of the llfi build directory  
You can set the environment variable llfibuild in your system to point it to the LLFI build directory in your local machine.   

Start the server:   
Go to the /web-app/server folder and run "node server.js"  

Start the front-end dev tool:   
Go to the web-app directory and run "webpack" or "webpack -w"   
-->

Results
-------
After fault injection, output from LLFI and the tested application can be found
in the *llfi* directory.

|     Directory         |                 Contents                       |
| ----------------------| ---------------------------------------------- |
| *std_output*          | Piped STDOUT from the tested application       |
| *llfi_stat_output*    | Fault injection statistics                     |
| *error_output*        | Failure reports (program crashes, hangs, etc.) |
| *trace_report_output* | Faults propogation report files and graph      |


References
----------
* [LLFI Paper](http://blogs.ubc.ca/karthik/2013/02/15/llfi-an-intermediate-code-level-fault-injector-for-soft-computing-applications/)
* [LLFI Wiki](https://github.com/DependableSystemsLab/LLFI/wiki)
* Udit Kumar Agarwal, Abraham Chan, Karthik Pattabiraman. LLTFI: Framework agnostic fault injection for machine learning applications (Tools and Artifacts Track). International Symposium on Software Reliability Engineering (ISSRE), 2022. 10 pages.   [LLTFI Paper](https://www.dropbox.com/s/lgr3ed75sy0fq2p/issre22-camera-ready.pdf?dl=0)

Citations
----------

<pre>
@article{Agarwal22LLTFI,
  title={LLTFI: Framework agnostic fault injection for machine learning applications (Tools and Artifacts Track)},
  author={Agarwal, Udit and Chan, Abraham and Pattabiraman, Karthik},
  journal={International Symposium on Software Reliability Engineering (ISSRE)},
  year={2022},
  publisher={IEEE}
}
</pre>

======		
Read *caveats.txt* for caveats and known problems.

