//
// Created by tzhou on 1/15/18.
//
#ifndef XPS_DomSetPass_H
#define XPS_DomSetPass_H

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
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "llvm/IR/Value.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/CFG.h"

using namespace llvm;

namespace xps {

typedef std::string string;

class DomSetPass: public FunctionPass {
  std::map<BasicBlock*, std::set<BasicBlock*>> _dom;
  bool _per_function;
public:
  static char ID;

  DomSetPass(): FunctionPass(ID) {
      // errs() << "Constructs DomSetPass.\n";
      initialize();
  }

  void initialize() {

  }

  //std::set<BasicBlock*>

  bool runOnFunction(Function& F) override {
    if (_per_function) {
      _dom.clear();
    }

    std::set<BasicBlock*> all_bbs;
    for (auto& B: F) {
      all_bbs.insert(&B);
    }

    for (auto& B: F) {
      BasicBlock* BB = &B;
      if (BB == &F.getEntryBlock()) {
        _dom[BB] = std::set<BasicBlock*>();
        _dom[BB].insert(BB);
      }
      else {
        _dom[BB] = all_bbs;
      }
    }

    bool changed = true;
    while (changed) {
      changed = false;
      for (auto& B: F) {
        BasicBlock* BB = &B;
        if (BB == &F.getEntryBlock()) {
          continue;
        }

        for (auto it = pred_begin(BB), et = pred_end(BB); it != et; ++it) {
          BasicBlock* predecessor = *it;
          auto& pred_dom_set = _dom[predecessor];
          std::set<BasicBlock*> tmp;
          std::set_intersection(_dom[BB].begin(), _dom[BB].end(),
                                pred_dom_set.begin(), pred_dom_set.end(),
                                std::inserter(tmp, tmp.begin()));
          tmp.insert(BB);
          if (_dom[BB] != tmp) {
            _dom[BB] = tmp;
            changed = true;
          }
        }
      }
    }

    return false;
  }

  void printDomSets() {
    errs() << "block num: " << _dom.size() << "\n";
    for (auto it: _dom) {
      auto key = it.first;
      auto set = it.second;

      string str = "{ ";
      for (BasicBlock* v: set) {
        str += v->getName().str() + ", ";
      }
      str += "}";
      errs() << key->getName() << ":\n";
      errs() << str << '\n';
    }
  }

  std::map<BasicBlock*, std::set<BasicBlock*>>* getDomSet(Function& F) {
    runOnFunction(F);
    return &_dom;
  };
};

char DomSetPass::ID = 0;

static RegisterPass<DomSetPass>
    DomSetPassInfo("dom-set", "Get dom set of each basic block",
                         false /* Only looks at CFG */,
                         true /* Analysis Pass */);
}


#endif