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

/// Back edge: edge n→h such that h dominates n
/// Natural loop of a back edge n→h:
/// - h is loop header
/// - Set of loop nodes is set of all nodes that can
///   reach n without going through h (and are
///   dominated by h)
/// Algorithm to identify natural loops in CFG:
/// - Compute dominator relation
/// - Identify back edges
/// – Compute the loop for each back edge

/// Property: for any two natural loops in the flow graph,
/// one of the following is true:
/// 1. They are disjoint
/// 2. They are nested
/// 3. They have the same header

class NaturalLoop {
public:
  BasicBlock *_header;
  std::set<BasicBlock *> _latches;
  std::set<BasicBlock *> _body;
  std::set<BasicBlock *> _exits;

  bool isLoopLatch(BasicBlock* BB) {
    return _latches.find(BB) != _latches.end();
  }

  BasicBlock* getHeader() {
    return _header;
  }

  std::set<BasicBlock *>& getBody() {
    return _body;
  }

  std::set<BasicBlock *>& getLatches() {
    return _latches;
  }
};

class LoopInfoPass : public FunctionPass {
  std::map<BasicBlock*, NaturalLoop *> _loops;
  std::set<BasicBlock*> _loop_body_visited_set;
  std::map<BasicBlock*, std::set<BasicBlock*>>* _dom;
  std::map<BasicBlock*, int> _starts;
  std::map<BasicBlock*, int> _finishes;
  int _time;
  bool _per_function;
public:
  static char ID;

  LoopInfoPass() : FunctionPass(ID) {
    // errs() << "Constructs LoopInfoPass.\n";
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

  void printBackEdge(BasicBlock* header, BasicBlock* latch) {
    outs() << "back edge: "
           << latch->getName()
           << " - >"
           << header->getName()
        ;

    if (isDominator(header, latch)) {
      outs() << " (dominator)";
    }
    outs() << "\n";
  }

  void DFSTraverse(BasicBlock* BB) {
    assert(_starts.find(BB) != _starts.end());

    _starts[BB] = _time++;
    for (auto it = succ_begin(BB), et = succ_end(BB);
         it != et; ++it) {
      BasicBlock* successor = *it;
      if (!isVisited(successor)) {
        DFSTraverse(successor);
      }
      else {
        /* Back edge identified */
        if (!isFinished(successor)) {
          BasicBlock* header = successor;
          if (isDominator(header, BB)) {
            createLoop(header, BB);
          }
        }
      }
    }
    _finishes[BB] = _time++;
  }

  void createLoop(BasicBlock* header, BasicBlock* latch) {
    /*  merge two loops that have the same header */
    if (NaturalLoop* L = _loops[header]) {
      L->_latches.insert(latch);
      loopGetBody(L, latch);
    }
    else {
      L = new NaturalLoop();
      L->_header = header;
      L->_latches.insert(latch);
      _loops[header] = L;
      loopGetBody(L, latch);
    }

    printLoop(_loops[header]);
    outs() << "\n";
  }

  // Find loop body via DFS
  void loopGetBody(NaturalLoop* loop, BasicBlock* root) {
    _loop_body_visited_set.clear();
    _loop_body_visited_set.insert(loop->_header);
    loopGetBodyImpl(loop, root);
  }

  bool loopBodyIsVisited(BasicBlock* BB) {
    return _loop_body_visited_set.find(BB)
        != _loop_body_visited_set.end();
  }

  void addToLoopBody(NaturalLoop* loop, BasicBlock* BB) {
    if (loop->isLoopLatch(BB)) {
      return;
    }
    loop->_body.insert(BB);
    _loop_body_visited_set.insert(BB);
  }

  void loopGetBodyImpl(NaturalLoop* loop, BasicBlock* root) {
    BasicBlock* header = loop->_header;
    addToLoopBody(loop, root);
    for (auto it = pred_begin(root), et = pred_end(root);
         it != et; ++it) {
      BasicBlock *predecessor = *it;
      if (!isDominator(header, predecessor)) {
        continue;
      }

      if (!loopBodyIsVisited(predecessor)) {
        loopGetBodyImpl(loop, predecessor);
      }
    }
  }

  void printLoop(NaturalLoop* L) {
    raw_ostream& OS = outs();
    OS << "<header>: ";
    L->getHeader()->printAsOperand(OS, false);
    OS << "\n<body>: ";
    for (auto& BB: L->getBody()) {
      BB->printAsOperand(OS, false);
      OS << ", ";
    }
    OS << "\n<latch>:";
    for (auto& BB: L->getLatches()) {
      BB->printAsOperand(OS, false);
      OS << ", ";
    }
    OS << "\n";
  }

  bool runOnFunction(Function &F) override {
    outs() << "F: " << F.getName() << "\n";
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
    LoopInfoPassInfo("loop-info", "Get loop info");

}


