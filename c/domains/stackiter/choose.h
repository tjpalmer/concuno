#ifndef stackiter_learner_chooser_h
#define stackiter_learner_chooser_h


#include "state.h"


/**
 * Turn all states into bags, keeping only alive items.
 *
 * The entity lists from the bags are provided separately, because sometimes we
 * do sub-state bags.
 *
 * TODO On the other hand, do I care about this function being interchangeable
 * TODO with other choose functions?
 */
cnBool stAllBagsFalse(
  cnList(stState)* states, cnList(cnBag)* bags,
  cnList(cnList(cnEntity)*)* entityLists
);


/**
 * Pick out states where blocks are dropped. Then choose positives where the
 * block dropped lands on another block (or at least doesn't land flat on
 * the ground).
 *
 * The entity lists from the bags are provided separately, because sometimes we
 * do sub-state bags.
 */
cnBool stChooseDropWhereLandOnOther(
  cnList(stState)* states, cnList(cnBag)* bags,
  cnList(cnList(cnEntity)*)* entityLists
);


/**
 * Create a bag for each live item, constraining it to be first bound. Each bag
 * is labeled true if the item's not moving.
 *
 * The entity lists from the bags are provided separately, because here we do
 * sub-state bags.
 */
cnBool stChooseWhereNotMoving(
  cnList(stState)* states, cnList(cnBag)* bags,
  cnList(cnList(cnEntity)*)* entityLists
);


#endif