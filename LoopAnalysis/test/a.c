#include <stdlib.h>

void foo() {
  
}

int main(int argc) {
  int i, j = 0;
  for (; i < argc*10; ++i) {
    for (; j < 10; ++j) {
      malloc(1);
    }
    malloc(1);
  }
  
  return 0;
}
