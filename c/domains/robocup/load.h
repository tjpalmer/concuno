#ifndef concuno_robocup_load_h
#define concuno_robocup_load_h


#include "domain.h"


/**
 * Loads the file indicated by the given name. Command logs usually end in
 * extension ".rcl".
 *
 * Adds action information to the already present states.
 */
cnBool cnrLoadCommandLog(cnrGame* game, char* name);


/**
 * Loads the file indicated by the given name. Game logs usually end in
 * extension ".rcg".
 */
cnBool cnrLoadGameLog(cnrGame* game, char* name);


#endif
