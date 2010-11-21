#ifndef cuncuno_predicate_h
#define cuncuno_predicate_h

#include "distrib.h"

namespace cuncuno {

/**
 * A boolean function on entities.
 *
 * TODO Make this a specialization of an abstract Function type.
 */
struct Predicate {

  /**
   * Returns whether or not the entity matches this predicate. The entity could
   * be composite.
   *
   * Errors, if any, will be thrown.
   */
  virtual bool operator ()(const void* entity) const = 0;

  // TODO Type and count.

};

// TODO ProbabilityThresholdPredicate without the Attribute/Function.

/**
 * A predicate explicitly based on a particular attribute, probability function,
 * and threshold. Making them explicit makes them tweakable.
 */
struct AttributePredicate: Predicate {

  /**
   * This is the same as the one-arg classify, except that error conditions are
   * provided via the error parameter instead of being thrown.
   *
   * TODO Provide an enum for tri-state bools?
   */
  virtual bool operator ()(const void* entity) const;

  /**
   * Allows extracting values from entities.
   */
  const Attribute* attribute;

  /**
   * Determines a probability of some value matching a concept. Extracted
   * attribute values are checked here.
   */
  ProbabilityFunction* probabilityFunction;

  /**
   * The threshold should be a probability that the entity matches the local
   * concept for this question. This could be chosen to maximize some objective.
   *
   * TODO Alternatively normalize PDFs such that the threshold is always 0.5.
   */
  Probability threshold;

};

}

#endif
