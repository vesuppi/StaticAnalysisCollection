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
  bool _per_function;
public:
  static char ID;

  LoopInfoPass() : FunctionPass(ID) {
    errs() << "Constructs LoopInfoPass.\n";
    initialize();
  }

  void initialize() {

  }

  bool runOnFunction(Function &F) override {
    if (_per_function) {
      _loops.clear();
    }

    return false;
  }
};
}