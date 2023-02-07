cmake_minimum_required(VERSION 2.8)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(llfi-rt)

add_library(llfi-rt SHARED
    CommonFaultInjectors.cpp
    FaultInjectionLib.c
    FaultInjectorManager.cpp
    InstTraceLib.c
    ProfilingLib.c
    Utils.c
    #_FIDLSoftwareFaultInjectors.cpp
    #_SoftwareFaultInjector.cpp is included in this file
)

# For ML backends. This static library is intended to be linked with the ML
# application, to provide fast FI.
add_library(ml-lltfi-rt
    MLFaultInjectionLib.cpp
)

add_executable(InjectorScanner
    InjectorScanner.cpp
)

TARGET_LINK_LIBRARIES(llfi-rt pthread)
TARGET_LINK_LIBRARIES(InjectorScanner llfi-rt)
