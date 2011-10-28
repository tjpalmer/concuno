#ifndef concuno_mat_h
#define concuno_mat_h


// Linear algebra (and other math?) stuff goes here.


// TODO Abstract out standard libraries?
#include <math.h>

#include "core.h"


namespace concuno {


/**
 * A measure of angle in radians.
 */
typedef Float Radian;


// TODO Something else on Windows?
#define cnPi ((Float)M_PI)


Float cnEuclideanDistance(Count size, Float* x, Float* y);


Float cnNorm(Count size, Float* x);


/**
 * Transforms target and result into a space where origin is the origin and
 * target is one unit along the first axis. The origin vector is unaffected.
 * The target is modified to avoid needing to allocate space.
 *
 * TODO Reconsider the modification of target.
 */
void cnReframe(Count size, Float* origin, Float* target, Float* result);


Float cnSquaredEuclideanDistance(Count size, Float* x, Float* y);


void cnVectorPrint(FILE* file, Count size, Float* values);


void cnVectorPrintDelimited(
  FILE* file, Count size, Float* values, const char* delimiter
);


/**
 * Normalizes radians to between -pi and pi.
 */
Radian cnWrapRadians(Radian angle);


}


#endif
