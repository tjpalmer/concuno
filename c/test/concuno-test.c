#include <concuno.h>
#include <stdio.h>
#include <stdlib.h>


// TODO Split stuff out.

// TODO Make a better test harness, too.


void testMultinomialSample(void);


void testUnitRand(void);


int main(void) {
  switch (2) {
  case 1:
    testUnitRand();
    break;
  case 2:
    testMultinomialSample();
    break;
  default:
    printf("No test requested.\n");
  }

  return EXIT_SUCCESS;
}


void testMultinomialSample(void) {
  cnIndex i;
  cnIndex j;
  cnCount out[4];
  cnFloat p[] = {0.2, 0.1, 0.4, 0.3};
  printf("testMultinomialSample:\n");
  for (j = 0; j < 4; j++) {
    printf("%lg ", p[j]);
  }
  printf("\n");
  for (i = 0; i < 10; i++) {
    cnMultinomialSample(4, out, 1000, p);
    for (j = 0; j < 4; j++) {
      printf("%ld ", out[j]);
    }
    printf("\n");
  }
}


void testUnitRand(void) {
  cnIndex i;
  // TODO Assert stuff, calculate statistics, and so on, instead of printing.
  printf("testUnitRand:\n");
  for (i = 0; i < 10; i++) {
    printf("%lg ", cnUnitRand());
  }
  printf("\n");
}
