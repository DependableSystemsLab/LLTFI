#ifndef LLFI_UTILS_H
#define LLFI_UTILS_H

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Constants.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

// For name demangling
#include <cxxabi.h>

#include <map>
#include <set>
#include <string>
#include <sstream>

using namespace llvm;
namespace llfi {
std::string intToString(int i);
std::string longToString(long i);

// Return a demangled version of a C++ function name. Removes type info from
// templated functions
std::string demangleFuncName(std::string func);

// return the terminate instruction of the function
Instruction *getTermInstofFunction(Function *func);
// return instumentation code insertion point for fi in reg of inst
Instruction *getInsertPtrforRegsofInst(Value *reg, Instruction *inst);

void getProgramExitInsts(Module &M, std::set<Instruction*> &exitinsts);

// get or set the LLFI index of the specified instruction. use metadata
long getLLFIIndexofInst(Instruction *inst);
void setLLFIIndexofInst(Instruction *inst);

// get the map of opcode name and their opcode
void genFullNameOpcodeMap(std::map<std::string, unsigned> &opcodenamemap);

//Check metadata to see if instruction was generated/inserted by LLFI
bool isLLFIIndexedInst(Instruction *inst);

// sets the metadata on the injectFault call
void setInjectFaultInst(Value *reg, Instruction *inst, Instruction *ficall);

// checks if the instruction is a call to llfi's 'injectFault*', if it is, return
// the next instruction iff injectFault occurs AFTER the targeted instruction
Instruction* changeInsertPtrIfInjectFaultInst(Instruction *inst);

//======== Add opcode_str QINING @SEP 13th========
GlobalVariable* findOrCreateGlobalNameString(Module &M, std::string name);
//================================================
}

#endif
