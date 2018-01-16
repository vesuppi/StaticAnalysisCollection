#include <stdlib.h>
#include <stdio.h>

void loop1() {
  int i = 0;
  for (; i < 3; ++i) {
    malloc(1);
  }
}

void loop2() {
  int i = 0;
  while (i < 3) {
    malloc(1);
    ++i;
  }
}

void loop3() {
  int i = 0;
  do {
    malloc(1);
    ++i;
  } while(i < 3);
}

void loop4() {
  int i, j = 0;
  for (; i < 3; ++i) {
    for (; j < 3; ++j) {
      malloc(1);
    }
    malloc(1);
  }
}

void loop5() {
  int i = 0;
  for (; i < 3; ++i) {
    if (i) {
      printf("i >= 1\n");
    }
    else {
      printf("i == 0\n");
    }
    malloc(1);
  }
}

int main(int argc) {

  
  return 0;
}
