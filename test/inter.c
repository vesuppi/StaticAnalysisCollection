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
