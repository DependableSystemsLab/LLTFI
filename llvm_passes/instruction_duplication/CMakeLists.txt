
add_llvm_library(SEDPasses MODULE
  InstructionDuplication.cpp
  
  PLUGIN_TOOL
  opt 
)
