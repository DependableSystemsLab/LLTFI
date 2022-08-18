#!/bin/bash
#
# LLFI_BUILD_ROOT must be set to the directory where LLFI is installed (same as that in setup)

rm -rf ./llfi*

fname=$1

# Generate Makefile
$LLFI_BUILD_ROOT/tools/GenerateMakefile --readable --all -o "$fname.ll"
make

# Instrument it
$LLFI_BUILD_ROOT/bin/instrument --readable "$fname.ll"

# Call the profiling script
shift
$LLFI_BUILD_ROOT/bin/profile ./llfi/"$fname-profiling.exe" $@

# Inject the faults
$LLFI_BUILD_ROOT/bin/injectfault ./llfi/"$fname-faultinjection.exe" $@

echo "Done injecting faults"
