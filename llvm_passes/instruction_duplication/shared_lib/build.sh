CLANG=${LLVM_GXX_BIN_DIR:+$LLVM_GXX_BIN_DIR/}clang
${CLANG} -shared SIDHelperFunctions.cpp -o libSIDHelperFunctions.so
