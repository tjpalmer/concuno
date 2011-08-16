#include <math.h>

#include "mat.h"


cnFloat cnEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y) {
  return sqrt(cnSquaredEuclideanDistance(size, x, y));
}


cnFloat cnSquaredEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y) {
  cnFloat distance = 0;
  cnFloat* xEnd = x + size;
  for (; x < xEnd; x++, y++) {
    cnFloat diff = *x - *y;
    distance += diff * diff;
  }
  return distance;
}


void cnVectorPrint(FILE* file, cnCount size, cnFloat* values) {
  cnVectorPrintDelimited(file, size, values, " ");
}


void cnVectorPrintDelimited(
  FILE* file, cnCount size, cnFloat* values, char* delimiter
) {
  // TODO Awesomer printing a la Matlab or better?
  cnFloat *value = values, *end = values + size;
  for (; value < end; value++) {
    if (value > values) {
      fprintf(file, "%s", delimiter);
    }
    fprintf(file, "%lg", *value);
  }
}
