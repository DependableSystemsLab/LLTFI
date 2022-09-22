cmake_minimum_required(VERSION 3.15)

option(NO_GUI "Skip building GUI" OFF)

add_subdirectory(llvm_passes)
add_subdirectory(runtime_lib)
add_subdirectory(bin)
add_subdirectory(tools)
add_subdirectory(config)
add_subdirectory(test_suite)
