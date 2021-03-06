#ifndef concuno_robocup_load_h
#define concuno_robocup_load_h


#include "domain.h"


namespace ccndomain {
namespace rcss {


/**
 * Loads the file indicated by the given name. Command logs usually end in
 * extension ".rcl".
 *
 * Adds action information to the already present states.
 */
bool cnrLoadCommandLog(Game* game, char* name);


/**
 * Loads the file indicated by the given name. Game logs usually end in
 * extension ".rcg".
 */
bool cnrLoadGameLog(Game* game, char* name);


}
}


#endif
