#ifndef concuno_robocup_choose_h
#define concuno_robocup_choose_h


#include "domain.h"


namespace ccndomain {
namespace rcss {


bool cnrChooseHoldsAndPasses(
  Game* game,
  concuno::List<concuno::Bag>* holdBags,
  concuno::List<concuno::Bag>* passBags,
  concuno::List<concuno::List<concuno::Entity>*>* entityLists
);


}
}


#endif
