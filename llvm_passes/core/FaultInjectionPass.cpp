//===- faultinjectionpass.cpp - Fault Injection Pass -==//
//
//                     LLFI Distribution
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This pass instruments selected registers of instruction with calls to fault
// injection libraries
//
// The fault injection function is a C function which performs the fault
// injection at runtime
// See faultinjection_lib.c injectFunc() function for more details on the 
// fault injection function. This function definition is linked to the 
// instrumented bitcode file (after this pass). 
//===----------------------------------------------------------------------===//
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>

#include "FaultInjectionPass.h"
#include "Controller.h"
#include "Utils.h"

namespace llfi {

char FaultInjectionPass::ID=0;

std::string FaultInjectionPass::getFIFuncNameforType(const Type *type) {
  std::string funcname;
  if (fi_rettype_funcname_map.find(type) != fi_rettype_funcname_map.end()) {
    funcname = fi_rettype_funcname_map[type];
  } else {
    funcname = "injectFault";
    int ficount = fi_rettype_funcname_map.size();	
    funcname += intToString(ficount);
    fi_rettype_funcname_map[type] = funcname;
  }
  return funcname;
}

void FaultInjectionPass::insertInjectionFuncCall(
    std::map<Instruction*, std::list< int >* > *inst_regs_map, Module &M) {

  for (std::map<Instruction*, std::list< int >* >::iterator inst_reg_it =
       inst_regs_map->begin(); inst_reg_it != inst_regs_map->end(); 
       ++inst_reg_it) {
    Instruction *fi_inst = inst_reg_it->first;
    
    std::list<int> *fi_reg_pos_list = inst_reg_it->second;
    /*BEHROOZ: This section makes sure that we do not instrument the intrinsic functions*/
    if(isa<CallInst>(fi_inst)){
      bool continue_flag=false;
      for (std::list<int>::iterator reg_pos_it_mem = fi_reg_pos_list->begin();
       	(reg_pos_it_mem != fi_reg_pos_list->end()) && (*reg_pos_it_mem != DST_REG_POS); ++reg_pos_it_mem) {
        std::string reg_mem =
            fi_inst->getOperand(*reg_pos_it_mem)->getName().str();
        if ((reg_mem.find("memcpy") != std::string::npos) || (reg_mem.find("memset") != std::string::npos) || (reg_mem.find("expect") != std::string::npos) || (reg_mem.find("memmove") != std::string::npos)){
          continue_flag=true;
          break;
        }
      }
      if(continue_flag)
        continue;
    }
    /*BEHROOZ: This is to make sure we do not instrument landingpad instructions.*/
    std::string current_opcode = fi_inst->getOpcodeName();
    if(current_opcode.find("landingpad") != std::string::npos){
      continue;
    }

    unsigned reg_index = 0;
    unsigned total_reg_num = fi_reg_pos_list->size();
    for (std::list<int>::iterator reg_pos_it = fi_reg_pos_list->begin(); 
         reg_pos_it != fi_reg_pos_list->end(); ++reg_pos_it, ++reg_index) {
      if(isa<GetElementPtrInst>(fi_inst)){
        GetElementPtrInst* gepi = dyn_cast<GetElementPtrInst>(fi_inst);
        gepi->setIsInBounds(false);
      }
      if(isa<CallInst>(fi_inst)){
        CallInst* ci = dyn_cast<CallInst>(fi_inst);
        ci->setTailCall(false);
      }

      Value* fi_reg = NULL;
      if(*reg_pos_it == DST_REG_POS)  fi_reg = fi_inst;
      else fi_reg = fi_inst->getOperand(*reg_pos_it);
      //if(isa<Constant>(fi_reg))  continue;
      Type *returntype = fi_reg->getType();
      
      // The return type is not a valid return type, and should hence be ignored
      // This is to deal with types such as Metadata that are not valid return types
      // Or if the instruction is an intrinsic one (starts with 'llvm_', ignore it
      if (! FunctionType::isValidReturnType(returntype) 
	 || isa<IntrinsicInst>(fi_inst) ) 
	      continue;

      // Get the context for the function
      LLVMContext &context = M.getContext();
      Type *i64type = Type::getInt64Ty(context);
      Type *i32type = Type::getInt32Ty(context);

      // function declaration
      std::vector<Type*> paramtypes(7);
      paramtypes[0] = i64type;	// llfi index
      paramtypes[1] = returntype;	// the instruction to be injected
      paramtypes[2] = i32type; // opcode
      paramtypes[3] = i32type; // current fi reg index
      paramtypes[4] = i32type; // total fi reg number
      //======== Add reg_pos QINING @DEC 23rd ==========
      paramtypes[5] = i32type;
      //================================================
      //======== Add opcode_str QINING @MAR 11th========
      paramtypes[6] = PointerType::get(Type::getInt8Ty(context), 0);
      //================================================

      //LLVM 3.3 Upgrade
      ArrayRef<Type*> paramtypes_array_ref(paramtypes);
      // dbgs() << "Getting function of type : " << *returntype <<"\n";
      FunctionType* injectfunctype = FunctionType::get(returntype, paramtypes_array_ref, false);

      std::string funcname = getFIFuncNameforType(returntype);
      FunctionCallee injectfunc =
          M.getOrInsertFunction(funcname, injectfunctype);

      // argument preparation for calling function
      // since the source register is another way of simulating fault
      // injection into "the instruction", use instruction's index instead
      Value *indexval = ConstantInt::get(i64type, getLLFIIndexofInst(fi_inst));

      std::vector<Value*> args(7);
      args[0] = indexval; //llfi index
      args[1] = fi_reg; // target register
      args[2] = ConstantInt::get(i32type, fi_inst->getOpcode()); // opcode in i32
      args[3] = ConstantInt::get(i32type, reg_index); // reg_index not reg_pos
      args[4] = ConstantInt::get(i32type, total_reg_num); // total_reg_num
      //======== Add reg_pos QINING @DEC 23rd ==========
      args[5] = ConstantInt::get(i32type, *reg_pos_it+1); // dstreg->0, operand0->1, operand1->2 ...
      //================================================
      //======== Add opcode_str QINING @MAR 11th========
      std::string opcode_str = fi_inst->getOpcodeName();
      GlobalVariable* opcode_str_gv = findOrCreateGlobalNameString(M, opcode_str);
      std::vector<Constant*> indices_for_gep(2);
      indices_for_gep[0] = ConstantInt::get(Type::getInt32Ty(context),0);
      indices_for_gep[1] = ConstantInt::get(Type::getInt32Ty(context),0);
      ArrayRef<Constant*> indices_for_gep_array_ref(indices_for_gep);
      Constant *opc_str = dyn_cast<Constant>(opcode_str_gv);
      Type *ty = opcode_str_gv->getValueType();
      Constant *gep_expr = ConstantExpr::getGetElementPtr(
          ty, opc_str, indices_for_gep_array_ref, true);
      args[6] = gep_expr; // opcode in string
      //================================================

      // LLVM 3.3 Upgrade
      ArrayRef<Value*> args_array_ref(args);

      Instruction *insertptr = getInsertPtrforRegsofInst(fi_reg, fi_inst);
      Instruction *ficall =
          CallInst::Create(injectfunc, args_array_ref, "fi", insertptr);
      setInjectFaultInst(fi_reg, fi_inst, ficall); // sets the instruction metadata

      // redirect the data dependencies
      if (fi_reg == fi_inst) {
        // inject into destination
        std::list<User*> inst_uses;
        for (Value::user_iterator user_it = fi_inst->user_begin();
             user_it != fi_inst->user_end(); ++user_it) {
          User *user = *user_it;

          if (user != ficall) {
            inst_uses.push_back(user);
          }
        }

        for (std::list<User *>::iterator user_it = inst_uses.begin();
             user_it != inst_uses.end(); ++user_it) {
          User *user = *user_it;
          user->replaceUsesOfWith(fi_inst, ficall);
          
          // update the selected inst pool
          /*if (Instruction *use_inst = dyn_cast<Instruction>(user)) {
            if (inst_regs_map->find(use_inst) != inst_regs_map->end()) {
              std::list<int> *reg_pos_list = (*inst_regs_map)[use_inst];
              
              for (std::list<int>::iterator reg_pos_it = reg_pos_list->begin();
                   reg_pos_it != reg_pos_list->end(); ++reg_pos_it) {
                if (use_inst->getOperand(*reg_pos_it) == fi_inst) {
                  use_inst->setOperand(*reg_pos_it, ficall);
                }
              }
            }
          }*/
        }

      } else {
        // inject into source
        //fi_inst->replaceUsesOfWith(fi_reg, ficall);
        fi_inst->setOperand(*reg_pos_it, ficall);
      }
    }
  }
}

void FaultInjectionPass::createInjectionFuncforType(
    Module &M, Type *fitype, std::string &fi_name, FunctionCallee injectfunc,
    FunctionCallee pre_fi_func) {
  LLVMContext &context = M.getContext();
  Function *f = M.getFunction(fi_name);
  std::vector<Value*> args;
  for(Function::arg_iterator ai = f->arg_begin(); ai != f->arg_end(); ++ai)
    args.push_back(&*ai);
  // args[0] llfi index, args[1] fault injection instruction
  // args[2] for opcode, args[3] for reg index, args[4] for total num of fi reg

  BasicBlock* entryblock = BasicBlock::Create(context, "entry", f);
  // store the value of target instruction to memory
  AllocaInst *tmploc = new AllocaInst(fitype, 0, "tmploc", entryblock);
  new StoreInst(args[1], tmploc, entryblock);

  std::vector<Value*> pre_fi_args(4);
  pre_fi_args[0] = args[0]; //LLFI index
  pre_fi_args[1] = args[2]; //opcode in i32
  pre_fi_args[2] = args[3]; //reg_index, not reg_pos!
  pre_fi_args[3] = args[4]; //total_reg_target_num

  // LLVM 3.3 Upgrade
  ArrayRef<Value*> pre_fi_args_array_ref(pre_fi_args);

  Value *prefuncval = CallInst::Create(pre_fi_func, pre_fi_args_array_ref, "pre_cond", entryblock);

  BasicBlock *fiblock = BasicBlock::Create(context, "inject", f);	
  BasicBlock *exitblock = BasicBlock::Create(context,"exit", f );
  //if prefuncval is true, goto inject function
  BranchInst::Create(fiblock, exitblock, prefuncval, entryblock);		
  BranchInst *fi2exit_branch = BranchInst::Create(exitblock, fiblock);

  std::vector<Value*> fi_args(6);
  fi_args[0] = args[0]; //LLFI index
  const DataLayout &td = M.getDataLayout();
  int size = td.getTypeSizeInBits(fitype);
  fi_args[1] = ConstantInt::get(Type::getInt32Ty(context), size); //size
  fi_args[2] = new BitCastInst(tmploc, 
                    PointerType::get(Type::getInt8Ty(context), 0),
                    "tmploc_cast", fi2exit_branch); //pointer to target memory
  fi_args[3] = args[3]; //reg_index not reg_pos!
  //======== Add reg_pos QINING @DEC 23rd ==========
  fi_args[4] = args[5]; // dstreg->0, operand0->1, operand1->2 ...
  //================================================
  //======== Add opcode_str QINING @MAR 11th========
  fi_args[5] = args[6]; //opcode in string
  //================================================
  ArrayRef<Value*> fi_args_array_ref(fi_args);
	
  CallInst::Create(injectfunc, fi_args_array_ref, "",
                   fi2exit_branch);

  LoadInst *updateval = new LoadInst(fitype, tmploc, "updateval", exitblock);
  ReturnInst::Create(context, updateval, exitblock);
}

void FaultInjectionPass::createInjectionFunctions(Module &M) {
  FunctionCallee pre_fi_func = getLLFILibPreFIFunc(M);
  FunctionCallee injectfunc = getLLFILibFIFunc(M);

  for (std::map<const Type*, std::string>::const_iterator fi = 
       fi_rettype_funcname_map.begin();
       fi != fi_rettype_funcname_map.end(); ++fi) {
    const Type *fi_type = fi->first;


    // LLVM 3.3 upgrading
    Type *fi_type_unconst = const_cast<Type*>(fi_type);

    std::string fi_name = fi->second;
    createInjectionFuncforType(M, fi_type_unconst, fi_name, injectfunc, pre_fi_func);
  }
}

bool FaultInjectionPass::runOnModule(Module &M) {
  checkforMainFunc(M);

  std::map<Instruction*, std::list< int >* > *fi_inst_regs_map;
  Controller *ctrl = Controller::getInstance(M);
  ctrl->getFIInstRegsMap(&fi_inst_regs_map);
  insertInjectionFuncCall(fi_inst_regs_map, M);

  finalize(M);
  return true;
}

void FaultInjectionPass::checkforMainFunc(Module &M) {
  Function* mainfunc = M.getFunction("main");
  if (mainfunc == NULL) {
    errs() << "ERROR: Function main does not exist, " <<
        "which is required by LLFI\n";
    exit(1);
  }
}

void FaultInjectionPass::finalize(Module &M) {
  Function *mainfunc = M.getFunction("main");
  BasicBlock *entryblock = &mainfunc->front();

  // function call for initInjections
  FunctionCallee initfunc = getLLFILibInitInjectionFunc(M);
  CallInst::Create(initfunc, "", entryblock->getFirstNonPHI());
  
  // function call for postInjections
  FunctionCallee postfifunc = getLLFILibPostInjectionFunc(M);

  std::set<Instruction*> exitinsts;
  getProgramExitInsts(M, exitinsts);
  assert (exitinsts.size() != 0 
            && "Program does not have explicit exit point");
  for (std::set<Instruction*>::iterator it = exitinsts.begin();
         it != exitinsts.end(); ++it) {
    Instruction *term = *it;
    CallInst::Create(postfifunc, "", term);
  }
	
  createInjectionFunctions(M);
}

FunctionCallee FaultInjectionPass::getLLFILibPreFIFunc(Module &M) {
  std::vector<Type*> pre_fi_func_param_types(4);
  LLVMContext& context = M.getContext();
  pre_fi_func_param_types[0] = Type::getInt64Ty(context);// index
  pre_fi_func_param_types[1] = Type::getInt32Ty(context);// opcode
  pre_fi_func_param_types[2] = Type::getInt32Ty(context);// my reg index
  pre_fi_func_param_types[3] = Type::getInt32Ty(context);// total reg index num

  // LLVM 3.3 Upgrade
  ArrayRef<Type*> pre_fi_func_param_types_array_ref(pre_fi_func_param_types);

  FunctionType *pre_fi_func_type = FunctionType::get(
      Type::getInt1Ty(context), pre_fi_func_param_types_array_ref, false);
  FunctionCallee pre_fi_func =
      M.getOrInsertFunction("preFunc", pre_fi_func_type);
  return pre_fi_func;
}

FunctionCallee FaultInjectionPass::getLLFILibFIFunc(Module &M) {
  LLVMContext& context = M.getContext();
  std::vector<Type*> fi_func_param_types(6);
  fi_func_param_types[0] = Type::getInt64Ty(context); // index
  fi_func_param_types[1] = Type::getInt32Ty(context); // size
  fi_func_param_types[2] = PointerType::get(Type::getInt8Ty(context), 0); //inst
  fi_func_param_types[3] = Type::getInt32Ty(context); // my reg index
  fi_func_param_types[4] = Type::getInt32Ty(context); // reg_pos
  //======== Add opcode_str QINING @MAR 11th========
  fi_func_param_types[5] = PointerType::get(Type::getInt8Ty(context), 0);
  //================================================

  // LLVM 3.3 Upgrade
  ArrayRef<Type*> fi_func_param_types_array_ref(fi_func_param_types);

  FunctionType *injectfunctype = FunctionType::get(
      Type::getVoidTy(context), fi_func_param_types_array_ref, false);
  FunctionCallee injectfunc =
      M.getOrInsertFunction("injectFunc", injectfunctype);
  return injectfunc;
}

FunctionCallee FaultInjectionPass::getLLFILibInitInjectionFunc(Module &M) {
  LLVMContext &context = M.getContext();
  FunctionType *fi_init_func_type =
      FunctionType::get(Type::getVoidTy(context), false);
  FunctionCallee initfunc =
      M.getOrInsertFunction("initInjections", fi_init_func_type);
  return initfunc;
}

FunctionCallee FaultInjectionPass::getLLFILibPostInjectionFunc(Module &M) {
  LLVMContext &context = M.getContext();
  FunctionType *postinjectfunctype =
      FunctionType::get(Type::getVoidTy(context), false);
  FunctionCallee postfifunc =
      M.getOrInsertFunction("postInjections", postinjectfunctype);
  return postfifunc;
}

static RegisterPass<FaultInjectionPass> X(
    "faultinjectionpass", "Fault injection pass", false, true);
}
