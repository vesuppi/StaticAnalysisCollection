#include <stdlib.h>

void foo() {
  
}

int main(int argc) {
  int i, j = 0;
  for (; i < argc*10; ++i) {
    malloc(1);
  }
  
  return 0;
}
