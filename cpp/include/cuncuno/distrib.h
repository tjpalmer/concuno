#ifndef cuncuno_distrib_h
#define cuncuno_distrib_h

#include "entity.h"

namespace cuncuno {

/**
 * A value that should be between 0 and 1, inclusive.
 */
typedef Float Probability;

/**
 * Represent the probability that an object belongs to some concept, not the
 * probability density (nor mass) of a given value coming out of a distribution.
 * This could be seen as a likelihood function. What's the likelihood of the
 * parameter here, or in other words, whats the probability of the entity
 * corresponding to the concept, given its values?
 *
 * TODO What's the right word for this?
 *
 * TODO Just use Function instead? Or make this a decorator?
 */
struct ProbabilityFunction {

  // TODO Type and count.

  /**
   * The probability that the entity matches this concept.
   */
  virtual Probability operator ()(const void* entity) const = 0;

};

// TODO Particular distributions.

// TODO SMRF tree probability function.

}

#endif
