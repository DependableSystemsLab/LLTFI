include(../config/llvm_passes.cmake)

set(LLVM_PASSES_DIRS_LLFI hardware_failures core software_failures)

include_directories(${LLVM_PASSES_DIRS_LLFI})


add_llvm_library(llfi-passes MODULE
  SampleFIInstSelector.cpp
  SampleFIRegSelector.cpp
  SoftwareFailureAutoScanPass.cpp
  HardwareFailureAutoScanPass.cpp
  MainGraphInstSelector.cpp
  CustomTensorOperatorInstSelector.cpp

  core/FaultInjectionPass.cpp
  core/InstTracePass.cpp
  core/LLFIDotGraphPass.cpp
  core/Utils.cpp
  core/Controller.cpp
  core/FICustomSelectorManager.cpp
  core/FIInstSelector.cpp
  core/FIInstSelectorManager.cpp
  core/FIRegSelector.cpp
  core/ProfilingPass.cpp
  core/GenLLFIIndexPass.cpp
  core/RegLocBasedFIRegSelector.cpp

  hardware_failures/FuncNameFIInstSelector.cpp
  hardware_failures/LLFIIndexFIInstSelector.cpp
  hardware_failures/InstTypeFIInstSelector.cpp

  RegisterPasses.cpp
)

add_subdirectory(./instruction_duplication)
