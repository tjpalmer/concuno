#ifndef cuncuno_learner_h
#define cuncuno_learner_h

#include "context.h"
#include "entity.h"
#include <vector>

namespace cuncuno {

/**
 * Represents a data sample for training or testing.
 *
 * Ideally, this will support both classification and multidimensional
 * regression.
 */
struct Sample {

  /**
   * TODO Some kind of argument identification among the entities in the bag.
   * TODO SMRF calls that h-pinning.
   */
  std::vector<const void*> entities;

  /**
   * TODO We need to be able to associate this with some kind of entity
   * TODO attribute. Sometimes, though, disconnected values will also be of
   * TODO use.
   */
  bool label;

};

/**
 * Allows setting learning parameters. The learn function can then be called on
 * multiple data sets.
 */
struct Learner: Worker {

  Learner(const Type& entityType);

  /**
   * TODO Do I really need this? For either validation or attributes?
   */
  const Type& entityType;

  /**
   * Functions take (arrays of) pointers to entities and return vectors or
   * categories or such.
   *
   * TODO How to pair with probability distributions?
   */
  std::vector<Function*> functions;

  /**
   * TODO What kind of return type?
   */
  void learn(const std::vector<Sample>& samples);

};

}

#endif
