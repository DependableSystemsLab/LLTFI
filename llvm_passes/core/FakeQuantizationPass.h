#ifndef LLFI_H
#define LLFI_H

#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"

#include <cxxabi.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace llfi {

extern llvm::cl::opt<std::string> fakeQuant;
extern llvm::cl::opt<int> minPercentileThreshold;
extern llvm::cl::opt<int> maxPercentileThreshold;
extern llvm::cl::opt<int> bitWidth;

extern std::unordered_set<llvm::Instruction *> deleteInst;
extern std::map<std::string, int64_t> ONNXOperatorId;
extern int totalNumberOfLayers;
extern int currentNumberOfLayers;

void findTotalLayers(llvm::Module &M);
void insertFakeQuantForInjectFault(llvm::Function &F);
void insertFakeQuantForProfiling(llvm::Function &F);
void insertFakeQuantInst(llvm::Module &M, bool IsProfiling);
std::string getOperandName(llvm::Instruction *I);

} // namespace llfi

#endif // LLFI_H
