#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h

#include "state.h"

namespace stackiter {

struct Chooser {
  void chooseDropWhereLandOnOtherTrue(
    const vector<State>& states,
    vector<State*>& pos,
    vector<State*>& neg
  );
};

}

#endif
