## A Collection of Static Analysis
This repo implements a number of fundamental static analysis
such as pointer analysis, liveness analysis, as LLVM passes.

The passes are intended to be run with `opt`.

I hope the code could be somewhat educational.

## Resolve Indirect Call
This pass only consider load, store and select for points-to set propagation,
so it should be used before SSA is constructed.
Otherwise phi node needs to be considered as well.