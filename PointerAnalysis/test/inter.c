typedef void (*fpty) ();

void foo() {
  printf("I am foo\n");
}

void call_func(fpty f) {
  f();
}

fpty get_func(fn) {
  return fn;
}

int main() {
  call_func(foo);
  get_func(foo)();
  return 0;
}
