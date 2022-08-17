#!/bin/sh

clang++ -S -fno-inline -fPIC -emit-llvm SIDHelperFunctions.cpp -o SIDHelperFunctions.ll -O3
