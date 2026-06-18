#!/bin/sh

CLANGXX=${LLVM_GXX_BIN_DIR:+$LLVM_GXX_BIN_DIR/}clang++
${CLANGXX} -S -fno-inline -fPIC -emit-llvm SIDHelperFunctions.cpp -o SIDHelperFunctions.ll -O3
