#include "llvm/IR/InstIterator.h"
#include "llvm/Support/raw_ostream.h"

#include "FIInstSelector.h"

namespace llfi {
void FIInstSelector::getFIInsts(Module &M, std::set<Instruction*> *fiinsts) {
  getInitFIInsts(M, fiinsts);

  std::set<Instruction* > bs;
  std::set<Instruction* > fs;
  // must do both of the computation on the fiinsts, and update
  // fiinsts finally
  if (includebackwardtrace)
    getBackwardTraceofInsts(fiinsts, &bs);
  if (includeforwardtrace)
    getForwardTraceofInsts(fiinsts, &fs);

  fiinsts->insert(bs.begin(), bs.end());
  fiinsts->insert(fs.begin(), fs.end());
}

void FIInstSelector::getInitFIInsts(Module &M, 
                                    std::set<Instruction*> *fiinsts) {
  for (Module::iterator m_it = M.begin(); m_it != M.end(); ++m_it) {
    if (!m_it->isDeclaration()) {
      // m_it is a function
      for (inst_iterator f_it = inst_begin(&*m_it); f_it != inst_end(&*m_it);
           ++f_it) {
        Instruction *inst = &(*f_it);
        if (isInstFITarget(inst)) {
          fiinsts->insert(inst);
        }
      }
    }  
  }
}

void FIInstSelector::getBackwardTraceofInsts(
    const std::set<Instruction* > *fiinsts, std::set<Instruction* > *bs) {
  for (std::set<Instruction* >::const_iterator inst_it = fiinsts->begin();
       inst_it != fiinsts->end(); ++inst_it) {
    Instruction *inst = *inst_it;
    getBackwardTraceofInst(inst, bs);
  }
}

void FIInstSelector::getForwardTraceofInsts(
    const std::set<Instruction* > *fiinsts, std::set<Instruction* > *fs) {
  for (std::set<Instruction* >::const_iterator inst_it = fiinsts->begin();
       inst_it != fiinsts->end(); ++inst_it) {
    Instruction *inst = *inst_it;
    getForwardTraceofInst(inst, fs);
  }
}

void FIInstSelector::getBackwardTraceofInst(Instruction *inst,
                                            std::set<Instruction*> *bs) {
  for (User::op_iterator op_it = inst->op_begin(); 
       op_it != inst->op_end(); ++op_it) {
    Value *src = *op_it;
    if (Instruction *src_inst = dyn_cast<Instruction>(src)) {
      if (bs->find(src_inst) == bs->end()) {
        bs->insert(src_inst);
        getBackwardTraceofInst(src_inst, bs);
      }
    }
  }
}

void FIInstSelector::getForwardTraceofInst(Instruction *inst,
                                           std::set<Instruction*> *fs) {
  for (Value::user_iterator user_it = inst->user_begin();
       user_it != inst->user_end(); ++user_it) {
    User *user = *user_it;
    if (Instruction *user_inst = dyn_cast<Instruction>(user)) {
      if (fs->find(user_inst) == fs->end()) {
        fs->insert(user_inst);
        getForwardTraceofInst(user_inst, fs);
      }
    }
  }
}

void FIInstSelector::getCompileTimeInfo(std::map<std::string, std::string>& info) {
  info["failure_class"] = "Unknown";
  info["failure_mode"] = "Unknown";
  info["targets"] = "Unknown";
  info["injector"] = "Unknown";
}

}
