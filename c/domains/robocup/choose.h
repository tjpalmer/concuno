#ifndef concuno_robocup_choose_h
#define concuno_robocup_choose_h


#include "domain.h"


bool cnrChooseHoldsAndPasses(
  cnrGame* game,
  concuno::cnList(concuno::Bag)* holdBags,
  concuno::cnList(concuno::Bag)* passBags,
  concuno::cnList(concuno::cnList(concuno::cnEntity)*)* entityLists
);


#endif
