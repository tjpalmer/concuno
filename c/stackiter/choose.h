#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h


#include "state.h"


/**
 * Turn all states into bags, keeping only alive items.
 */
cnBool stAllBagsFalse(cnList(stState)* states, cnList(cnBag)* bags);


/**
 * Pick out states where blocks are dropped. Then choose positives where the
 * block dropped lands on another block (or at least doesn't land flat on
 * the ground).
 */
cnBool stChooseDropWhereLandOnOther(
  const cnList(stState)* states, cnList(cnBag)* bags
);


#endif
