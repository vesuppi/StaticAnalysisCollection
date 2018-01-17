//
// Created by tzhou on 1/17/18.
//

#ifndef XPS_InstSetName_H
#define XPS_InstSetName_H

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

using namespace llvm;

typedef std::string string;

namespace xps {
// It seems I can't iterate loops in a module pass
class InstSetNamePass: public ModulePass {
  std::map<string, size_t> _ops;
public:
  static char ID;

  InstSetNamePass(): ModulePass(ID) {
    errs() << "Constructs InstSetNamePass.\n";
    initialize();
  }

  void initialize() {

  }

  size_t getOpCount(string op) {
    return _ops[op];
  }

  void incOpCount(string op) {
    if (_ops.find(op) != _ops.end()) {
      _ops[op]++;
    }
    else {
      _ops[op] = 0;
    }
  }

  void nameInsts(Module& M) {
    for (auto& F: M) {
      for (auto& B: F) {
        for (auto& I: B) {
          if (dyn_cast<LoadInst>(&I)
              || dyn_cast<CastInst>(&I)) {
            string op = I.getOpcodeName();
            incOpCount(op);
//            outs() << op
//                   << " " << getOpCount(op)
//                   << '\n';
            I.setName(op+std::to_string(getOpCount(op)));
          }
        }
      }
    }
  }

  bool runOnModule(Module& M) override {
    return true;
  }
};

char InstSetNamePass::ID = 0;

static RegisterPass<InstSetNamePass>
    InstSetNamePassInfo("inst-set-name", "",
                         false /* Only looks at CFG */,
                         true /* Analysis Pass */);
}

#endif