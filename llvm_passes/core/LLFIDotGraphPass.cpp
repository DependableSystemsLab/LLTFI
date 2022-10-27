#include <vector>
#include <cmath>
#include <string>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Value.h"
#include "Utils.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#define DATADEPCOLOUR "blue"

using namespace llvm;

#include "LLFIDotGraphPass.h"

namespace llfi {

struct instNode {
  std::string name, label;
  Instruction *raw;
  std::string dotNode();
  instNode(Instruction *target);
};

instNode::instNode(Instruction *target) {
  raw = target;

  long llfiID = llfi::getLLFIIndexofInst(target);
  name = "llfiID_" + longToString(llfiID);
  FILE *outputFile = fopen("llfi.index.map.txt", "a");

  label = std::string(" [shape=record,label=\"") + longToString(llfiID);
  label += std::string("\\n") + target->getOpcodeName() + "\\n";
  DebugLoc dbgLoc = target->getDebugLoc();
  if (bool(dbgLoc) && dbgLoc.getLine()) {
    label += "(Line #: " + intToString(dbgLoc.getLine()) + ")\\n";
    /* if (MDNode *N= target->getMetadata("dbg")){
       label += "(In File: " + DILocation (N).getFilename().str().substr(DILocation (N).getFilename().str().find_last_of("/\\")+1)+")";
    } */
    if (outputFile)
      fprintf(outputFile, "%s line_%s\n", name.c_str(),intToString(target->getDebugLoc().getLine()).c_str()); 
  } 
  else{
    if (outputFile)
      fprintf(outputFile, "%s line_N/A\n", name.c_str());
  }
  label += "\"]";
}

std::string instNode::dotNode() {
  return name + label;
}

struct bBlockGraph {
  BasicBlock* raw;
  std::string name;
  std::string funcName;
  std::vector<instNode> instNodes;
  Instruction* entryInst;
  Instruction* exitInst;
  bBlockGraph(BasicBlock *target);
  bool addInstruction(Instruction* inst);
  bool writeToStream(std::ofstream &target);
};

bBlockGraph::bBlockGraph(BasicBlock *BB) {
  raw = BB;
  name = BB->getName().str();
  funcName = BB->getParent()->getName().str();
  BasicBlock::iterator lastInst;
  for (BasicBlock::iterator instIterator = BB->begin(),
     lastInst = BB->end();
     instIterator != lastInst;
     ++instIterator) {

    Instruction *inst = &*instIterator;

    addInstruction(inst);
  }
  entryInst = &(BB->front());
  exitInst = &(BB->back());
}
bool bBlockGraph::addInstruction(Instruction* inst) {
  instNodes.push_back(instNode(inst));

  return true;
}

bool bBlockGraph::writeToStream(std::ofstream &target) {
  target << "subgraph \"cluster_" << funcName << "_" << name << "\" {\n";
  target << "label = \"" << funcName << "_" << name << "\";\n";
  for (unsigned int i = 0; i < instNodes.size(); i++) {
    target << instNodes.at(i).dotNode() << ";\n";
  }
  target << "}\n";
  for (unsigned int i = 1; i < instNodes.size(); i++) {
    target << instNodes.at(i-1).name << " -> " << instNodes.at(i).name << ";\n";
  }
  return true;
}


bool llfiDotGraph::runOnFunction(Function &F) {
  //Create handles to the functions parent module and context
  LLVMContext &context = F.getContext();

  std::vector<bBlockGraph> blocks;

  Function::iterator lastBlock;
  //iterate through each basicblock of the function
  for (Function::iterator blockIterator = F.begin(), lastBlock = F.end();
    blockIterator != lastBlock; ++blockIterator) {

    BasicBlock* block = &*blockIterator;

    bBlockGraph b(block);
    blocks.push_back(b);
  }
  for (unsigned int i = 0; i < blocks.size(); i++) {
    bBlockGraph currBlock = blocks.at(i);
    for (unsigned int i = 0; i < currBlock.instNodes.size(); i++) {
      Instruction *inst = currBlock.instNodes.at(i).raw;
      std::string nodeName = currBlock.instNodes.at(i).name;
      instNode node = currBlock.instNodes.at(i);
      if (!inst->use_empty()) {
        // TODO: optimize the algorithm below later
  // Iterates over the uses of instruction and finds their basic blocks and annotates them
        for (Value::use_iterator useIter = inst->use_begin();
             useIter != inst->use_end(); useIter++) {
          Value* userValue = *useIter;
          for (unsigned int f = 0; f < blocks.size(); f++) {
            bBlockGraph searchBlock = blocks.at(f);
            for (unsigned int d = 0; d < searchBlock.instNodes.size(); d++) {
              Instruction* targetInst = searchBlock.instNodes.at(d).raw;
              if (userValue == targetInst) {
                instNode targetNode = searchBlock.instNodes.at(d);
                outfs << nodeName << " -> " << targetNode.name;
                outfs << " [color=\"" << DATADEPCOLOUR << "\"];\n";
              }
            }
          }
        }
      }
    }
  }

  for (unsigned int i = 0; i < blocks.size(); i++) {
    bBlockGraph block = blocks.at(i);
    block.writeToStream(outfs);
    if (block.exitInst->getOpcode() == Instruction::Br) {
      BranchInst* exitInst = (BranchInst*)block.exitInst;
      for (unsigned int s = 0; s < exitInst->getNumSuccessors(); s++) {
        BasicBlock* succ = exitInst->getSuccessor(s);
        for (unsigned int d = 0; d < blocks.size(); d++) {
          if (blocks.at(d).raw == succ) {
            std::string from = block.instNodes.back().name;
            std::string to = blocks.at(d).instNodes.front().name;
            outfs << from << " -> "  << to << ";\n";
          }
        }
      }
    }
  }

  return false;
}

//Register the pass with the llvm
char llfiDotGraph::ID = 0;
static RegisterPass<llfiDotGraph> X("dotgraphpass",
  "Outputs a dot graph of instruction execution at runtime", false, false);

}
