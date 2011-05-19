#ifndef cuncuno_mat_h
#define cuncuno_mat_h

#include "entity.h"
#include "grid.h"

namespace cuncuno {

void diff(Float* out, const Float* a, const Float* b, Count ndim);

/**
 * Finds the min of the values passed in. The type provides both the count of
 * vectors as well as the dimensionality of each.
 *
 * TODO Generic functions based on provided math ops. I'm still not a big
 * TODO template person.
 */
void min(Float* out, const Float* in, Count count, Count ndim);

void min(GridOf<Float>& out, const GridOf<Float>& in, Count dim);

void sum(Float* out, const Float* a, const Float* b, Count ndim);

}

#endif
