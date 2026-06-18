cmake_minimum_required(VERSION 3.5)

include(../config/copy_utils.cmake)

project(test_suite)

copydir(HardwareFaults HardwareFaults)
copydir(PROGRAMS PROGRAMS)
copydir(Traces Traces)
copydir(MakefileGeneration MakefileGeneration)
copy(test_suite.yaml test_suite.yaml)

add_subdirectory(SCRIPTS)

genCopy()

