// Copyright (C) 2023 Intel Corporation (HDFIT components)
// SPDX-License-Identifier: Apache-2.0

#ifndef FAULTINJECTION_PASS_H
#define FAULTINJECTION_PASS_H

#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <iostream>
#include <list>
#include <map>
#include <string>

using namespace llvm;

namespace llfi {
  // For Legacy PM.
  class FaultInjectionPass: public ModulePass {
   public:
    FaultInjectionPass() : ModulePass(ID) { }
    virtual bool runOnModule(Module &M);
    static char ID;

   private:
// HDFIT: we need this method to return a bool and not abort compilation
    bool checkforMainFunc(Module &M);
// ---------------------------------------------------------------------
    void finalize(Module& M);

    void insertInjectionFuncCall(
        std::map<Instruction*, std::list< int >* > *inst_regs_map, Module &M);
    void createInjectionFuncforType(Module &M, Type *functype,
                                    std::string &funcname, FunctionCallee fi_func,
                                    FunctionCallee pre_func);
    void createInjectionFunctions(Module &M);

  private:
// HDFIT: adding Module as input to disambiguate FI function names
    std::string getFIFuncNameforType(const Type* type, Module &M);
// --------------------------------------------------------------

    FunctionCallee getLLFILibPreFIFunc(Module &M);
    FunctionCallee getLLFILibFIFunc(Module &M);
    FunctionCallee getLLFILibInitInjectionFunc(Module &M);
    FunctionCallee getLLFILibPostInjectionFunc(Module &M);

  private:
    std::map<const Type*, std::string> fi_rettype_funcname_map;
  };

  // For New PM
  struct NewFaultInjectionPass:  llvm::PassInfoMixin<NewFaultInjectionPass> {
    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &){

      auto obj = new FaultInjectionPass();
      bool isChanged = obj->runOnModule(M);

      delete obj;
      return (isChanged) ? llvm::PreservedAnalyses::none():
                           llvm::PreservedAnalyses::all();
    }

    // Without isRequired returning true, this pass will be skipped for functions
    // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
    // all functions with optnone.
    static bool isRequired() { return true; }
  };
}
#endif
