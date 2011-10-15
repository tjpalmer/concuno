#include <math.h>

#include "mat.h"


cnFloat cnEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y) {
  return sqrt(cnSquaredEuclideanDistance(size, x, y));
}


cnFloat cnNorm(cnCount size, cnFloat* x) {
  cnFloat norm = 0;
  cnFloat* xEnd = x + size;
  for (; x < xEnd; x++) {
    norm += *x * *x;
  }
  return sqrt(norm);
}


void cnReframe(
  cnCount size, cnFloat* origin, cnFloat* target, cnFloat* result
) {
  cnIndex i;

  // Translate. This is the first easy part.
  for (i = 0; i < size; i++) {
    target[i] -= origin[i];
    result[i] -= origin[i];
  }

  // Rotate. This is the hard part. We rotate through one plane at a time,
  // always axis aligned for convenience, and always including the first axis,
  // where we want to retain a nonzero value in target. Whatever we apply to
  // target, we also apply to result, so's to get the same transform.
  //
  // Here, i represents the dimension being zeroed in target. It starts at the
  // second dimension (dim 1). This means no rotation for one-dimensional data,
  // and that seems okay.
  //
  // TODO Is there value (stability or otherwise) in sorting the dimensions in
  // TODO some fashion before doing the rotations, rather than going in
  // TODO (arbitrary) increasing order by dimension index?
  //
  // TODO In learning and propagation with many bindings, the same
  // TODO frame-defining pair will be used repeatedly. If we could cache the
  // TODO frame calculation, we reapply it, saving some CPU time. Is it enough
  // TODO to matter? How to coordinate such caching? Thread-local blackboards?
  // TODO Meanwhile, the operations for calculation here are few.
  //
  for (i = 1; i < size; i++) {
    // Focus just on the (0, i) plane.
    //
    // This part is what uses the stable calculation recommendations at
    // Wikipedia. I haven't usually paid attention to stability elsewhere.
    // TODO Think on that? Can this be simplified?
    cnFloat x = target[0];
    cnFloat y = target[i];
    cnFloat cosine;
    cnFloat sine;
    cnFloat radius;

    // Calculate the rotation.
    if (!y) {
      // Already aligned on first axis.
      // TODO Copysign alternative on windows?
      cosine = copysign(1, x);
      sine = 0.0;
      radius = fabs(x);
    } else if (!x) {
      // Aligned on axis i, and not first at all.
      sine = -copysign(1, y);
      cosine = 0.0;
      radius = fabs(y);
    } else if (fabs(x) >= fabs(y)) {
      // At Wikipedia, ratio is t, and radius before being finished is u.
      cnFloat ratio = x / y;
      radius = copysign(sqrt(1 + ratio * ratio), y);
      sine = -1.0 / radius;
      cosine = -sine * ratio;
      radius *= y;
    } else {
      // Alternative for |y| > |x|.
      cnFloat ratio = y / x;
      radius = copysign(sqrt(1 + ratio * ratio), x);
      cosine = 1.0 / radius;
      sine = -cosine * ratio;
      radius *= x;
    }

    // Apply the rotation.
    // The new target falls out automatically.
    target[0] = radius;
    target[i] = 0;
    // The result needs the rotation applied. Reuse x for a temp var.
    // TODO Back to stability, what happens to all these repeated changes to
    // TODO result[0]? If this matters, could stacking pairs up help?
    x = result[0];
    result[0] = x * cosine - result[i] * sine;
    result[i] = x * sine + result[i] * cosine;
  }

  // Scale. This is the other easy part.
  for (i = 0; i < size; i++) {
    result[i] /= target[0];
  }
  target[0] = 1.0;
}


cnFloat cnSquaredEuclideanDistance(cnCount size, cnFloat* x, cnFloat* y) {
  // Doesn't use generic norm so as not to allocate a temporary diff vector.
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


cnRadian cnWrapRadians(cnRadian angle) {
  angle = fmod(angle, 2 * cnPi);
  if (angle < -cnPi) {
    angle += 2 * cnPi;
  } else if (angle > cnPi) {
    angle -= 2 * cnPi;
  }
  return angle;
}
