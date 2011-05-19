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

  stTypeId typeId;

} stItem;

typedef struct stState {

  cnBool cleared;

  cnList items;

  /**
   * Simulation time, actually.
   *
   * TODO Break this out as a separately typed item?
   */
  double time;

} stState;

#endif
