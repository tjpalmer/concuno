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
bool stAllBagsFalse(
  concuno::List<stState>* states, concuno::List<concuno::Bag>* bags,
  concuno::List<concuno::List<concuno::Entity>*>* entityLists
);


/**
 * Pick out states where blocks are dropped. Then choose positives where the
 * block dropped lands on another block (or at least doesn't land flat on
 * the ground).
 *
 * The entity lists from the bags are provided separately, because sometimes we
 * do sub-state bags.
 */
bool stChooseDropWhereLandOnOther(
  concuno::List<stState>* states, concuno::List<concuno::Bag>* bags,
  concuno::List<concuno::List<concuno::Entity>*>* entityLists
);


/**
 * Create a bag for each live item, constraining it to be first bound. Each bag
 * is labeled true if the item's not moving.
 *
 * The entity lists from the bags are provided separately, because here we do
 * sub-state bags.
 */
bool stChooseWhereNotMoving(
  concuno::List<stState>* states, concuno::List<concuno::Bag>* bags,
  concuno::List<concuno::List<concuno::Entity>*>* entityLists
);


#endif
