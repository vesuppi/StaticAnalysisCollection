//
// Created by tzhou on 1/15/18.
//


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
#include "DomSetPass.h"

using namespace llvm;

namespace xps {

typedef std::string string;

class NaturalLoop {
public:
  BasicBlock *_header;
  BasicBlock *_preheader;
  std::set<BasicBlock *> _latches;
  std::set<BasicBlock *> _body;
  std::set<BasicBlock *> _exits;

};

class LoopInfoPass : public FunctionPass {
  std::vector<NaturalLoop *> _loops;
  std::map<BasicBlock*, std::set<BasicBlock*>>* _dom;
  bool _per_function;
  std::map<BasicBlock*, int> _starts;
  std::map<BasicBlock*, int> _finishes;
  int _time;
public:
  static char ID;

  LoopInfoPass() : FunctionPass(ID) {
    errs() << "Constructs LoopInfoPass.\n";
    _time = 0;
    initialize();
  }

  void initialize() {

  }

  void initAux(Function &F) {
    _time = 0;
    _starts.clear();
    _finishes.clear();

    for (auto& B: F) {
      _starts[&B] = -1;
      _finishes[&B] = -1;
    }
  }

  bool isVisited(BasicBlock* BB) {
    assert(_starts.find(BB) != _starts.end());
    return _starts[BB] != -1;
  }

  bool isFinished(BasicBlock* BB) {
    assert(_finishes.find(BB) != _finishes.end());
    return _finishes[BB] != -1;
  }

  bool isDominator(BasicBlock* dom, BasicBlock* BB) {
    assert(_dom->find(BB) != _dom->end());
    auto dom_set = _dom->at(BB);
    return dom_set.find(dom) != dom_set.end();
  }

  void DFSTraverse(BasicBlock* BB) {
    assert(_starts.find(BB) != _starts.end());

    _starts[BB] = _time++;
    for (auto it = succ_begin(BB), et = succ_end(BB); it != et; ++it) {
      BasicBlock* successor = *it;
      if (!isVisited(successor)) {
        DFSTraverse(successor);
      }
      else {
        if (!isFinished(successor)) {
          outs() << "back edge: "
                 << BB->getName()
                 << " - >"
                 << successor->getName()
              ;

          if (isDominator(successor, BB)) {
            outs() << " (dominator)"
                   ;
          }
          outs() << "\n";
        }
      }
    }
    _finishes[BB] = _time++;
  }

  bool runOnFunction(Function &F) override {
    if (_per_function) {
      _loops.clear();
    }

    initAux(F);
    auto domSetPass = new DomSetPass();
    _dom = domSetPass->getDomSet(F);
    DFSTraverse(&F.getEntryBlock());

    return false;
  }
};

char LoopInfoPass::ID = 0;

static RegisterPass<LoopInfoPass>
    LoopInfoPassInfo("loop-info", "Get loop info",
                     false /* Only looks at CFG */,
                     true /* Analysis Pass */);

}


