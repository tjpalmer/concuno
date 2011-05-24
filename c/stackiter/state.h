#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h


#include <cuncuno.h>


typedef enum {
  None,
  Block,
  Tool,
  Tray,
  View,
  World,
} stItemType;


typedef int stId;


typedef int stTypeId;


typedef struct stItem {

  cnBool alive;

  /**
   * Only ever true for blocks.
   *
   * This is somewhat cheating. Maybe ignore it.
   */
  cnBool grasped;

  /**
   * Only ever true for tools.
   */
  cnBool grasping;

  stId id;

  cnFloat location[2];

  stTypeId typeId;

} stItem;


typedef struct stState {

  cnBool cleared;

  cnList items;

  double time;

} stState;


void stStateDispose(stState* state);


void stStateInit(stState* state);


#endif
