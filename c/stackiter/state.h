#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h


#include <cuncuno.h>


typedef enum {
  stTypeNone,
  stTypeBlock,
  stTypeTool,
  stTypeTray,
  stTypeView,
  stTypeWorld,
} stType;


typedef cnIndex stId;


typedef int stTypeId;


typedef struct stItem {

  cnBool alive;

  cnFloat color[3];

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


void stItemInit(stItem* item);


void stStateDispose(stState* state);


void stStateInit(stState* state);


#endif
