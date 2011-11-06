#ifndef stackiter_learner_loader_h
#define stackiter_learner_loader_h

#include "state.h"


namespace ccndomain {namespace stackiter {


/**
 * The states list should have been previously init'd.
 */
bool load(char* name, concuno::List<State>* states);


}}


#endif
