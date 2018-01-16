//
// Created by tzhou on 1/14/18.
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

using namespace llvm;

namespace xps {

typedef std::string string;

class HelloLoopPass: public LoopPass {
public:
  static char ID;

  HelloLoopPass(): LoopPass(ID) {

  }

  bool runOnLoop(Loop * L, LPPassManager &LPM) override {
      raw_ostream& OS = errs();
      BasicBlock *H = L->getHeader();
      for (unsigned i = 0; i < L->getBlocks().size(); ++i) {
          BasicBlock *BB = L->getBlocks()[i];
          if (i) OS << ",";
          BB->printAsOperand(OS, false);

          if (BB == H) OS << "<header>";
          if (L->isLoopLatch(BB)) OS << "<latch>";
          if (L->isLoopExiting(BB)) OS << "<exiting>";
      }
      OS << "\n";
  }
};

char HelloLoopPass::ID = 0;

static RegisterPass<HelloLoopPass>
    HelloLoopPassInfo("hello-loop", "Simple loop info display",
                   false /* Only looks at CFG */,
                   true /* Analysis Pass */);


// It seems I can't iterate loops in a module pass
class LoopFunctionPass: public FunctionPass {
public:
  static char ID;

  LoopFunctionPass(): FunctionPass(ID) {
      errs() << "Constructs LoopFunctionPass.\n";
      initialize();
  }

  void initialize() {

  }

  bool runOnFunction(Function& F) override {
      /* This only compiles in runOnFunction */
      LoopInfoWrapperPass &wrapperPass = getAnalysis<LoopInfoWrapperPass>();
      LoopInfo *LI = &wrapperPass.getLoopInfo();
      DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      for (Loop *L: *LI) {
          /* do stuff */
      }
  }
};

char LoopFunctionPass::ID = 0;

static RegisterPass<LoopFunctionPass>
    LoopFunctionPassInfo("hello-function-loop", "Iterate loops in a function pass",
                            false /* Only looks at CFG */,
                            true /* Analysis Pass */);
}