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
#include "llvm/IR/Value.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

/// This analysis uses the workqueue algorithm from
/// https://www.seas.harvard.edu/courses/cs252/2011sp/slides/Lec06-PointerAnalysis.pdf

namespace xps {

typedef std::string string;

// static cl::opt<string> AndersenOpt
// ("ander-test-opt", cl::desc("AndersenOpt"), cl::value_desc("value"));

class SpaceValue: public Value {
public:
  explicit SpaceValue(Type* ty): Value(ty, 0) {}
  void dump() const {
    errs() << "SpaceValue: " << this << '\n';
  }
};

class ResolveIndiCallPass: public ModulePass {
public:
  Module* _m = nullptr;
  // All points-to sets map
  std::map<Value*, std::set<Value*>*> _pts;
  std::map<Value*, std::set<Value*>*> _graph;
  int _opt_level = 0;
public:
  static char ID;

  ResolveIndiCallPass(): ModulePass(ID) {
    errs() << "Constructs ResolveIndiCallPass.\n";
    initialize();
  }

  // Do some non module-specific stuff
  void initialize() {

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
    errs() << v1 << " to " << v2 << '\n';
    auto s1 = getPointToSet(v1);
    auto s2 = getPointToSet(v2);
    size_t size_before = s2->size();
    s2->insert(s1->begin(), s1->end());
    size_t size_after = s2->size();

    // Update the transitive closure
    if (size_before != size_after) {
      for (auto V: *getAdjacentNodes(v2)) {
        propagatePts(v1, V);
      }
    }
  }

  void doCall(Function& F) {
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

  void doAlloca(AllocaInst* I) {
    getPointToSet(I)->insert(
        new SpaceValue(I->getAllocatedType()));
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
    copyPointToSet(pointer, I);
  }

  void doAssignment(Function& F) {
    for (auto& BB: F) {
      for (auto& I: BB) {
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
        else if (CallSite CS = CallSite(&I)) {

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
          if (Instruction* I = dyn_cast<Instruction>(key)) {
            I->dump();
            errs() << "    points to @"
                   << F->getName() << '\n';
          }
        }
      }
    }
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
      string funcs = "{ ";
      for (Value* v: *set) {
        if (auto F = dyn_cast<Function>(v)) {
          funcs += "@" + F->getName().str() + ", ";
        }
      }
      funcs += "}";

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


    for (auto& F: M) {
      //if (F.getName().equals("main")) {
        doAssignment(F);//}
    }

    errs() << "to report\n";
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
