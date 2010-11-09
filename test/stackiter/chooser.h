#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h

#include <cuncuno.h>
#include "state.h"

namespace stackiter {

typedef cuncuno::Sample<const Item*, bool> BooleanItemSample;

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
    const std::vector<State>& states, std::vector<BooleanItemSample>& samples
  );

  /**
   * Finds items grasped in the state, and returns whether any were found.
   */
  bool findGraspedItems(
    const State& state, std::vector<const Item*>* items=0
  );

};

}

#endif
