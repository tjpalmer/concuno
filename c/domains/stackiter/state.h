#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h


#include <concuno.h>


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

  bool alive;

  cnFloat color[3];

  cnFloat extent[2];

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

  cnFloat orientation;

  cnFloat orientationVelocity;

  stType type;

  cnFloat velocity[2];

} stItem;


typedef struct stState {

  cnBool cleared;

  cnList(stItem) items;

  double time;

} stState;


void stItemInit(stItem* item);


/**
 * Inits the schema from scratch to have everything needed for stackiter data.
 *
 * Dispose with cnSchemaDispose (since nothing special needed here).
 */
cnBool stSchemaInit(cnSchema* schema);


void stStateDispose(stState* state);


stItem* stStateFindItem(stState* state, stId id);


void stStateInit(stState* state);


cnBool stStateCopy(stState* to, stState* from);


#endif
