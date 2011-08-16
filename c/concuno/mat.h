#ifndef concuno_mat_h
#define concuno_mat_h


// Linear algebra stuff goes here.


// TODO Abstract out standard libraries?
#include <math.h>

#include "core.h"


// TODO Something else on Windows?
#define cnPi M_PI


cnFloat cnEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


cnFloat cnSquaredEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


void cnVectorPrint(FILE* file, cnCount size, cnFloat* values);


void cnVectorPrintDelimited(
  FILE* file, cnCount size, cnFloat* values, char* delimiter
);


#endif
