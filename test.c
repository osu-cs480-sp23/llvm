#include "stdio.h"

extern float foo();

int main() {
  float x = foo();
  printf("Return from foo(): %f\n", x);
}
