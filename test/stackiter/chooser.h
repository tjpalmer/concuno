#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h

#include "state.h"

namespace stackiter {

struct Chooser {
  void chooseDropWhereLandOnOtherTrue(
    const vector<const State>& states,
    vector<const State&> pos,
    vector<const State&> neg
  );
};

}

#endif
