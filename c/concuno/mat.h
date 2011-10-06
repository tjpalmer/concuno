#ifndef concuno_mat_h
#define concuno_mat_h


// Linear algebra (and other math?) stuff goes here.


// TODO Abstract out standard libraries?
#include <math.h>

#include "core.h"


cnCBegin;


/**
 * A measure of angle in radians.
 */
typedef cnFloat cnRadian;


// TODO Something else on Windows?
#define cnPi ((cnFloat)M_PI)


cnFloat cnEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


cnFloat cnNorm(cnCount size, cnFloat* x);


cnFloat cnSquaredEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y);


void cnVectorPrint(FILE* file, cnCount size, cnFloat* values);


void cnVectorPrintDelimited(
  FILE* file, cnCount size, cnFloat* values, char* delimiter
);


/**
 * Normalizes radians to between -pi and pi.
 */
cnRadian cnWrapRadians(cnRadian angle);


cnCEnd;


#endif
