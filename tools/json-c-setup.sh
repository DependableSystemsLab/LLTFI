#!/bin/sh

echo "Obtaining json-c from Github"
git clone https://github.com/json-c/json-c
cd json-c
git checkout 42aa6f7257a42
mkdir build
cd build

echo "Building json-c library"
cmake -G Ninja ..
ninja -j10 -k10

echo " \n\n\n Installing json-c library. Please enter your password: \n\n"
sudo ninja install
