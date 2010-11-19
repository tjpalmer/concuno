#ifndef cuncuno_distrib_h
#define cuncuno_distrib_h

#include "entity.h"

namespace cuncuno {

/**
 * A value that should be between 0 and 1, inclusive.
 */
typedef Float Probability;

/**
 * Actually, I want this to represent the probability that an object belongs to
 * some concept, not the probability density of a given value coming out.
 *
 * TODO What's the right word for this?
 */
struct ProbabilityFunction {

  // TODO Type and count.

  /**
   * The probability that the entity matches this concept.
   */
  virtual Probability evaluate(const void* entity) const = 0;

};

// TODO Particular distributions.

// TODO SMRF tree probability function.

}

#endif
