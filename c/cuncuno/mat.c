#include <stdio.h>
#include "mat.h"


void cnVectorPrint(cnCount size, cnFloat* values){
  // TODO Awesomer printing a la Matlab or better?
  cnFloat *value = values, *end = values + size;
  for (; value < end; value++) {
    if (value > values) {
      printf(" ");
    }
    printf("%le", *value);
  }
}
