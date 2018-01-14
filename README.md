## A Collection of Static Analysis
This repo implements a number of fundamental static analysis
such as pointer analysis, liveness analysis, as LLVM passes.

The passes are intended to be run with `opt`.

I hope the code could be somewhat educational.

## Resolve Indirect Call
This pass only consider load, store and select for points-to set propagation,
so it should be used before SSA is constructed.
Otherwise phi node needs to be considered as well.

### Example
Go to PointerAnalysis/test/, do
```bash
$ cat inter.c
typedef void (*fpty) ();

void foo() {
  printf("I am foo\n");
}

void call_func(fpty f) {
  f();
}

int main() {
  call_func(foo);
  return 0;
}
$ clang -S -emit-llvm -g inter.c
$ opt -load /xxx/libXPSAnalysis.so -resolve-indi inter.ll > /dev/null
```

This will output:
```
inter.c:8 -> { @foo, }
```
The output indicates that line 8 in inter.c has an indirect call to `foo`.