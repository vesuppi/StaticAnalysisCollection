//
// Created by tzhou on 1/9/18.
//

#ifndef LLVM_PointerAnalysis_H
#define LLVM_PointerAnalysis_H

#include <cmath>
#include <set>
#include <llvm/Support/raw_ostream.h>
#include <fstream>
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfo.h"
#include <llvm/IR/CallSite.h>
#include <llvm/IR/Constants.h>
#include <llvm/Transforms/Utils/MemorySSA.h>
#include "llvm/IR/Value.h"
#include "llvm/IR/PassManager.h"
#include "InstSetName.h"

using namespace llvm;

/// This analysis uses the workqueue algorithm from
/// https://www.seas.harvard.edu/courses/cs252/2011sp/slides/Lec06-PointerAnalysis.pdf

namespace xps {

typedef std::string string;

static cl::opt<bool> PrintPropagation
("print-propagation", cl::desc("PrintPropagation"), cl::value_desc("bool"));

class SpaceValue: public Value {
  string _name;
  int _id;
public:
  static int ID;
public:
  explicit SpaceValue(Type* ty): Value(ty, 0) {
    _id = ID++;
  }

  int getID() {
    return _id;
  }

  string getName() {
    return "S" + std::to_string(_id);
  }
};

int SpaceValue::ID = 0;

class ResolveIndiCallPass: public ModulePass {
public:
  Module* _m = nullptr;
  // All points-to sets map
  std::map<Value*, std::set<Value*>*> _pts;
  std::map<Value*, std::set<Value*>*> _graph;
  std::map<Value*, string> _mem_ref_names;
  int _opt_level = 0;
  bool _print_propagation = false;
public:
  static char ID;

  ResolveIndiCallPass(): ModulePass(ID) {
    errs() << "Constructs ResolveIndiCallPass.\n";
    initialize();
  }

  // Do some non module-specific stuff
  void initialize() {
    if (PrintPropagation) {
      _print_propagation = true;
    }
  }

  void initPts(Module& M) {
    for (auto& F: M) {
      if (F.isIntrinsic()) {
        continue;
      }
      std::set<Value*>* singleton_set = new std::set<Value*>();
      singleton_set->insert(&F);
      _pts[&F] = singleton_set;
    }

    for (auto& GV: M.getGlobalList()) {
      if (GV.getName().startswith(".str")) {
        continue;
      }

      Type* value_ty = GV.getValueType();
      getPointToSet(&GV)->insert(new SpaceValue(value_ty));

//          if (value_ty->isPointerTy())
    }
  }


  std::set<Value*>* getPointToSet(Value* key) {
    if (_pts.find(key) == _pts.end()) {
      _pts[key] = new std::set<Value*>();
    }

    return _pts[key];
  }

  void copyPointToSet(Value* from, Value* to) {
    _pts[to] = getPointToSet(from);
  }

  std::set<Value*>* getAdjacentNodes(Value* key) {
    if (_graph.find(key) == _graph.end()) {
      _graph[key] = new std::set<Value*>();
    }

    return _graph[key];
  }

  void addEdge(Value* v1, Value* v2) {
    auto adjs = getAdjacentNodes(v1);
    adjs->insert(v2);
    propagatePts(v1, v2);
  }

  void propagatePts(Value* v1, Value* v2) {
    auto s1 = getPointToSet(v1);
    auto s2 = getPointToSet(v2);
    size_t size_before = s2->size();
    s2->insert(s1->begin(), s1->end());
    size_t size_after = s2->size();
    if (_print_propagation) {
      printPointsToSet(v2);
    }

    // Update the transitive closure
    if (size_before != size_after) {
      for (auto V: *getAdjacentNodes(v2)) {
        propagatePts(v1, V);
      }
    }
  }

  void doCalls(Function& F) {
    for (auto& BB: F) {
      for (auto &I: BB) {
        if (CallSite CS = CallSite(&I)) {
          Function* callee = CS.getCalledFunction();
          if (!callee || callee->isIntrinsic()) {
            continue;
          }

          errs() << "f: " << callee->getName() << '\n';
          int arg_num = CS.getNumArgOperands();
          int param_num = callee->getArgumentList().size();
          assert(arg_num == param_num);

          Function::arg_iterator PI = callee->arg_begin(), PE = callee->arg_end();
          CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();
          for (; AI != AE; ++AI, ++PI) {
            Value* A = *AI;
            if (auto CE = dyn_cast<ConstantExpr>(A)) {
              A = CE->getOperand(0);  // Get the function
            }
            addEdge(A, &*PI);
          }
        }
      }
    }
  }

  // Propagate arguments and return value
  void doCall(CallSite CS) {
    Function* callee = CS.getCalledFunction();
    if (!callee) {
      return;
    }

    if (callee->isIntrinsic() || callee->isDeclaration()) {
      return;
    }

    if (callee->isVarArg()) {
      errs().changeColor(raw_ostream::MAGENTA);
      errs() << "<WARNING>: ";
      errs().resetColor();
      errs() << "Ignored VarArg function ["
             << callee->getName() << "]"
             << "\n";
    }

    int arg_num = CS.getNumArgOperands();
    int param_num = callee->getArgumentList().size();
    assert(arg_num == param_num);

    // Propagate arguments
    Function::arg_iterator PI = callee->arg_begin(), PE = callee->arg_end();
    CallSite::arg_iterator AI = CS.arg_begin(), AE = CS.arg_end();

    for (; AI != AE; ++AI, ++PI) {
      Value* A = *AI;
      if (auto CE = dyn_cast<ConstantExpr>(A)) {
        A = CE->getOperand(0);  // Get the function
      }
      addEdge(A, &*PI);
    }

    // Propagate return value
    Type* VoidTy = Type::getVoidTy(_m->getContext());
    if (callee->doesNotReturn() || callee->getReturnType() == VoidTy) {
      return;
    }

    for (auto& B: *callee) {
      for (auto& I: B) {
        if (ReturnInst* R = dyn_cast<ReturnInst>(&I)) {
          assert(R->getNumOperands() == 1);
          addEdge(getRealOperand(R, 0), CS.getInstruction());
        }
      }
    }
  }

  void doAlloca(AllocaInst* I) {
    auto stack_space = new MemoryDef(I->getContext(), nullptr,
                                     I, I->getParent(), ++SpaceValue::ID);
    //auto stack_space = new SpaceValue(I->getAllocatedType());
    if (I->hasName()) {
      //stack_space->setName("*"+I->getName());
      string name = std::to_string(SpaceValue::ID);
      name += " (*%" + I->getName().str() + ")";
      _mem_ref_names[stack_space] = "S" + name;
    }

    getPointToSet(I)->insert(stack_space);
  }

  void doLoad(LoadInst* I) {
    // Load value from <pointer> to <result>
    // %2 = load %1
    // _pts[%2] = { _pts[v] for v in _pts[%1] }
    Value* result = I;
    Value* pointer = I->getPointerOperand();
    if (auto CE = dyn_cast<ConstantExpr>(pointer)) {
      pointer = CE->getOperand(0);
    }

    for (auto V: *getPointToSet(pointer)) {
      addEdge(V, result);
    }
  }

  void doStore(StoreInst* I) {
    // Store <value> to <pointer>
    Value* value = I->getValueOperand();
    if (auto CE = dyn_cast<ConstantExpr>(value)) {
      value = CE->getOperand(0);  // Get the function
    }
    Value* pointer = I->getPointerOperand();
    if (auto CE = dyn_cast<ConstantExpr>(pointer)) {
      pointer = CE->getOperand(0);  // Get the function
    }

    for (auto V: *getPointToSet(pointer)) {
      addEdge(value, V);
    }
  }

  void doSelect(SelectInst* I) {
    Value* v1 = getRealOperand(I, 1);
    Value* v2 = getRealOperand(I, 2);
    addEdge(v1, I);
    addEdge(v2, I);
  }

  void doGEP(GetElementPtrInst* I) {
    Value* pointer = I->getPointerOperand();
    // copyPointToSet(pointer, I);
    addEdge(pointer, I);
  }

  void doCast(CastInst* I) {
    assert(I->getNumOperands() == 1);
    addEdge(getRealOperand(I, 0), I);
  }

  void doAssignment(Function& F) {
    for (auto& BB: F) {
      for (auto& I: BB) {
        I.dump();
        if (auto i = dyn_cast<AllocaInst>(&I)) {
          doAlloca(i);
        }
        else if (auto i = dyn_cast<LoadInst>(&I)) {
          doLoad(i);
        }
        else if (auto i = dyn_cast<StoreInst>(&I)) {
          doStore(i);
        }
        else if (auto i = dyn_cast<SelectInst>(&I)) {
          doSelect(i);
        }
        else if (auto i = dyn_cast<GetElementPtrInst>(&I)) {
          doGEP(i);
        }
        else if (auto i = dyn_cast<CastInst>(&I)) {
          doCast(i);
        }
        else if (CallSite CS = CallSite(&I)) {
          doCall(CS);
        }
      }
    }
  }

  Value* getRealOperand(Instruction* I, uint i) {
    Value* v = I->getOperand(i);
    if (auto CE = dyn_cast<ConstantExpr>(v)) {
      return CE->getOperand(0);
    }
    else {
      return v;
    }
  }

  void printPointsToMap() {
    for (auto it: _pts) {
      auto key = it.first;
      auto set = it.second;
      for (Value* v: *set) {
        if (auto F = dyn_cast<Function>(v)) {
          if (auto I = dyn_cast<Instruction>(key)) {
            errs() << key->getName() << " points to @"
                   << F->getName() << '\n';
          }
        }
      }
    }
  }

  string& getMemoryDefName(MemoryDef* M) {
    assert(_mem_ref_names.find(M) != _mem_ref_names.end());
    return _mem_ref_names[M];
  }

  void printPointsToSet(Value* key) {
    auto set = getPointToSet(key);
    errs() << "    | ";
    if (auto M = dyn_cast<MemoryDef>(key)) {
      errs() << getMemoryDefName(M);
    }
    else {
      if (key->hasName()) {
        errs() << key->getName();
      }
      else {

        errs() << "this";
      }
    }

    errs() << " -> " << getSetAsStr(set)
           << "\n";
  }

  void printPointeePointsToSet(Value* key) {
    auto set = getPointeePointsToSet(key);
    errs() << "    | ";
    if (key->hasName()) {
      errs() << key->getName();
    }
    else {
      errs() << "__";
    }

    errs() << " -> &" << getSetAsStr(set)
           << "\n";
  }

  std::set<Value*>* getPointeePointsToSet(Value* key) {
    auto set = new std::set<Value*>();
    auto pointees = getPointToSet(key);
    for (auto p: *pointees) {
      for (auto v: *getPointToSet(p)) {
        set->insert(v);
      }
    }
    return set;
  }

  string getSetAsStr(std::set<Value*>* set) {
    string str = "{ ";
    for (Value* v: *set) {
      if (auto F = dyn_cast<Function>(v)) {
        str += "@" + F->getName().str() + ", ";
      }
      else if (auto M = dyn_cast<MemoryDef>(v)) {
        //str += M->getName().str() + ", ";
        str += _mem_ref_names[M] + ", ";
      }
    }
    size_t len = str.size();
    if (len > 2) {
      str[len-2] = ' ';
      str[len-1] = '}';
    }
    else {
      str += "}";
    }
    return str;
  }

  string getLocAsStr(Instruction* I) {
    string s;
    if (DILocation* loc = I->getDebugLoc()) {
      s += loc->getFilename();
      s += ":" + std::to_string(loc->getLine());
    }
    else {
      errs() << "No debug info found, exit.\n";
      exit(0);
    }
    return s;
  }

  void report() {
    errs() << _pts.size() << '\n';
    for (auto it: _pts) {
      auto key = it.first;
      auto set = it.second;

      string funcs = getSetAsStr(set);
      // todo: this repeats one user forever
      for (auto U: key->users()) {
        if (auto cs = CallSite(U)) {
          /* Indirect call */
          if (!cs.getCalledFunction()) {
            string loc = getLocAsStr(cs.getInstruction());
            errs() << loc << " -> " << funcs << '\n';
          }
        }
      }
    }
  }

  bool runOnModule(Module& M) override {
    _m = &M;

    initPts(M);
    InstSetNamePass* pass = new InstSetNamePass();
    pass->nameInsts(M);

    for (auto& F: M) {
      if (_print_propagation) {
        errs() << "Function: " << F.getName() << "\n";
      }
      doAssignment(F);

    }

    report();
    return false;
  }
};

char ResolveIndiCallPass::ID = 0;

static RegisterPass<ResolveIndiCallPass>
    ResolveIndiCallPassInfo("resolve-indi",
                            "Try to resolve the indirect calls for a given module",
                            false /* Only looks at CFG */,
                            true /* Analysis Pass */);
}

#endif //LLVM_PointerAnalysis_H
