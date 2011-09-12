#ifndef stackiter_learner_loader_h
#define stackiter_learner_loader_h

#include "state.h"


/**
 * The states list should have been previously init'd.
 */
cnBool stLoad(char* name, cnList(stState)* states);


#endif
