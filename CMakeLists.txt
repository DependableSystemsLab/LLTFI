cmake_minimum_required(VERSION 3.15)

option(NO_GUI "Skip building GUI" OFF)

# HDFIT: uncomment for double-precision FP instrumentation, single-precision otherwise
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHDFIT_DOUBLE")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHDFIT_DOUBLE")

# HDFIT: uncomment to perform the FI pass as first during optimization, last otherwise
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHDFIT_FIRSTOPT")

#HDFIT: uncomment to inline LLTFI function calls (enabled by default)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHDFIT_INLINE")

#HDFIT: uncomment to perform one additional srand() call at FI time
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHDFIT_SRAND")

add_subdirectory(llvm_passes)
add_subdirectory(runtime_lib)
add_subdirectory(bin)
add_subdirectory(tools)
add_subdirectory(config)
add_subdirectory(test_suite)
