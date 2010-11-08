#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h

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
    const std::vector<State>& states,
    std::vector<const State*>& pos,
    std::vector<const State*>& neg
  );

  bool findGraspedItems(const State& state, std::vector<const Item*>* items=0);

};

}

#endif
