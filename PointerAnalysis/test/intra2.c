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
  fpty fp;
  int i = argc;
  printf("i: %d\n", i);
  if (argc > 1) {
    i = power(2, (double)i);
    fp = foo;
  }
  else {
    i += 2;
    fp = bar;
  }
  call_func(fp);
  fp();
  return i;
}
