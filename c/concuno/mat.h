#ifndef concuno_mat_h
#define concuno_mat_h


// Linear algebra stuff goes here.


// Try to abstract out these standard libraries?
#include <math.h>
#include <stdio.h>

#include "core.h"


// TODO Something else on Windows?
#define cnPi M_PI


cnFloat cnEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


cnFloat cnSquaredEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


void cnVectorPrint(FILE* file, cnCount size, cnFloat* values);


#endif
