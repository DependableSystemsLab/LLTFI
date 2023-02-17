cmake_minimum_required(VERSION 2.8)

project(llfi-rt)

# HDFIT: using -O3 in an attempt to improve runtime performance
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
# ------------------------------------------------------------

add_library(llfi-rt SHARED 
    CommonFaultInjectors.cpp
    FaultInjectionLib.c
    FaultInjectorManager.cpp
    InstTraceLib.c
    ProfilingLib.c
    Utils.c
    _FIDLSoftwareFaultInjectors.cpp #_SoftwareFaultInjector.cpp is included in this file
)

add_executable(InjectorScanner
    InjectorScanner.cpp
)

TARGET_LINK_LIBRARIES(llfi-rt pthread)
TARGET_LINK_LIBRARIES(InjectorScanner llfi-rt)
