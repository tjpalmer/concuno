#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h

#include <cuncuno.h>
#include "state.h"

namespace stackiter {

/**
 * Various helper functions for sifting out data for testing cuncuno.
 */
struct Chooser {

  /**
   * Pick out states where blocks are dropped. Then choose positives where the
   * block dropped lands on another block (or at least doesn't land flat on
   * the ground).
   */
  void chooseDropWhereLandOnOtherTrue(
    const std::vector<State>& states, std::vector<cuncuno::BoolSample>& samples
  );

  /**
   * Finds items grasped in the state, and returns whether any were found.
   */
  bool findGraspedItems(
    const State& state, std::vector<const Item*>* items=0
  );

};

bool onGround(const Item& item);

double norm(const double* values, size_t count);

/**
 * Place pointers to alive items into the entities vector.
 */
void placeLiveItems(
  const std::vector<Item>& items, std::vector<const cuncuno::Entity*>& entities
);

}

#endif
