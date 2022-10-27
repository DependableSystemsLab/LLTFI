#include <iostream>
#include <fstream>
#include "llvm/Support/raw_os_ostream.h"

namespace llfi{

  struct llfiDotGraph : public FunctionPass {
    static char ID;
    std::ofstream outfs;
    llfiDotGraph() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &M) {
      outfs.open("llfi.stat.graph.dot", std::ios::trunc);
      outfs << "digraph \"LLFI Program Graph\" {\n";

      return false;
    }

    virtual bool doFinalization(Module &M) {
      outfs << "{ rank = sink;"
        "Legend [shape=none, margin=0, label=<"
         "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">"
         " <TR>"
         "  <TD COLSPAN=\"2\"><B>Legend</B></TD>"
         " </TR>"
         " <TR>"
         "  <TD>Correct Control Flow</TD>"
         "  <TD><FONT COLOR=\"black\"> solid arrow </FONT></TD>"
         " </TR>"
         " <TR>"
         "  <TD>Data Dependancy</TD>"
         "  <TD><FONT COLOR=\"blue\"> solid arrow </FONT></TD>"
         " </TR>"
         " <TR>"
         "  <TD>Error Propogation Flow</TD>"
         "  <TD><FONT COLOR=\"red\">solid arrow </FONT></TD>"
         " </TR>"
         " <TR>"
         "  <TD>The Affected Instruction(s) by Fault Injection  </TD>"
         "  <TD BGCOLOR=\"YELLOW\"></TD>"
         " </TR>"
         " <TR>"
         "  <TD>The Instruction(s) LLFI Injects Faults to</TD>"
         "  <TD BGCOLOR=\"red\"></TD>"
         " </TR>"
         "</TABLE>"
       ">];"
       "}";
      outfs << "}\n";
      outfs.close();
      return false;
    }

    virtual bool runOnFunction(Function &F);
  };

  struct NewLLFIDotGraph : llvm::PassInfoMixin<NewLLFIDotGraph> {

    // Without isRequired returning true, this pass will be skipped for functions
    // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
    // all functions with optnone.
    static bool isRequired() { return true; }

    // Main entry point, takes IR unit to run the pass on (&F) and the
    // corresponding pass manager (to be queried if need be)
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
      llfiDotGraph tempObj;
      tempObj.doInitialization(M);

      for (Function &F : M) {
        tempObj.runOnFunction(F);
      }

      tempObj.doFinalization(M);
      return PreservedAnalyses::all();
    }
  };
}
