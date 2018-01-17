#include <stdlib.h>

typedef void (*fpty) ();

void foo() {
  printf("I am foo\n");
}

void bar() {
  printf("I am bar\n");
}

void call_func(fpty f) {
  f();
}

int main(int argc, char** argv) {
  fpty fp1 = bar;
  fpty fp2 = foo;
  fp1 = fp2;
  fp2 = fp1;

  call_func(fp1);
  call_func(fp2);
}
