// Copyright (C) 2023 Intel Corporation (HDFIT components)
// SPDX-License-Identifier: Apache-2.0

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"

#include "core/ProfilingPass.h"
#include "core/GenLLFIIndexPass.h"
#include "core/FaultInjectionPass.h"
#include "core/LLFIDotGraphPass.h"
#include "core/InstTracePass.h"

using namespace llvm;

namespace llfi {

  //-----------------------------------------------------------------------------
  // New PM Registration
  //-----------------------------------------------------------------------------
  llvm::PassPluginLibraryInfo getLLFIPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "llfi_passes", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {

// HDFIT: We only need the genllfiindex and faultinjection passes
// Registered as last to not interfere with vectorization and such
// These two registration calls DO NOT impact opt runs, but only
// affect the clang frontend when using LLTFI
#ifndef HDFIT_FIRSTOPT
              PB.registerOptimizerLastEPCallback(
#else // HDFIT_FIRSTOPT
              PB.registerPipelineStartEPCallback(
#endif
                  [](ModulePassManager &MPM, OptimizationLevel) {
                      MPM.addPass(llfi::GenLLFIIndexPass());
                      MPM.addPass(llfi::NewFaultInjectionPass());
#ifdef HDFIT_INLINE
		      MPM.addPass(AlwaysInlinerPass());
#endif
                      return true;
                  });
// ---------------------------------------------------------------

              // For GenLLFIIndexPass
              PB.registerPipelineParsingCallback(
                  [](StringRef Name, ModulePassManager &MPM,
                     ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "genllfiindexpass") {
                      MPM.addPass(llfi::GenLLFIIndexPass());
                      return true;
                    }
                    return false;
                  });

              // For ProfilingPass
              PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "profilingpass") {
                    MPM.addPass(llfi::ProfilingPass());
                    return true;
                  }
                  return false;
                });

              // For FaultInjectionPass
              PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "faultinjectionpass") {
                    MPM.addPass(llfi::NewFaultInjectionPass());
                    return true;
                  }
                  return false;
                });

              // For DotGraphPass
              PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "dotgraphpass") {
                    MPM.addPass(llfi::NewLLFIDotGraph());
                    return true;
                  }
                  return false;
                });

              // For InstructionTracePass
              PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "insttracepass") {
                    MPM.addPass(llfi::NewInstTrace());
                    return true;
                  }
                  return false;
                });
            }};
  }

  extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
  llvmGetPassPluginInfo() {
    return getLLFIPassPluginInfo();
  }
}
