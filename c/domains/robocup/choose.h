#ifndef concuno_robocup_choose_h
#define concuno_robocup_choose_h


#include "domain.h"


bool cnrChooseHoldsAndPasses(
  cnrGame* game,
  cnList(cnBag)* holdBags, cnList(cnBag)* passBags,
  cnList(cnList(cnEntity)*)* entityLists
);


#endif
