#ifndef concuno_robocup_choose_h
#define concuno_robocup_choose_h


#include "domain.h"


bool cnrChooseHoldsAndPasses(
  cnrGame* game,
  concuno::cnList(cnBag)* holdBags, concuno::cnList(cnBag)* passBags,
  concuno::cnList(cnList(cnEntity)*)* entityLists
);


#endif
