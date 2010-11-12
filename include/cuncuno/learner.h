#ifndef cuncuno_learner_h
#define cuncuno_learner_h

#include "entity.h"
#include <vector>

namespace cuncuno {

/**
 * Represents a data sample for training or testing.
 *
 * Ideally, this will support both classification and multidimensional
 * regression.
 */
template<typename Value>
struct Sample {

  /**
   * TODO Some kind of argument identification among the entities in the bag.
   * TODO SMRF calls that h-pinning.
   */
  std::vector<const Entity*> entities;

  /**
   * TODO We need to be able to associate this with some kind of entity
   * TODO attribute. Sometimes, though, disconnected values will also be of
   * TODO use.
   */
  Value value;

};

typedef Sample<bool> BoolSample;

/**
 * TODO What kind of return type?
 * TODO What should this be named?
 * TODO How to provide a schema?
 */
void learn(const std::vector<BoolSample>& samples);

}

#endif
