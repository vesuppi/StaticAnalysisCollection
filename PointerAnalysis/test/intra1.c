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

  if (argc > 1) {
    fp = foo;
  }
  else {
    fp = bar;
  }
  call_func(fp);
}
