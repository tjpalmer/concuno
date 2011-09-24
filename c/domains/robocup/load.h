#ifndef concuno_robocup_load_h
#define concuno_robocup_load_h


#include "domain.h"


/**
 * Loads the file indicated by the given name. Game logs usually end in
 * extension ".rcg".
 *
 * TODO What to load it into? For now, just parse.
 */
cnBool cnrLoadGameLog(cnList(struct cnrState)* states, char* name);


#endif
