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


typedef concuno::Index stId;


typedef int stTypeId;


struct stItem {

  bool alive;

  concuno::Float color[3];

  concuno::Float extent[2];

  /**
   * Only ever true for blocks.
   *
   * This is somewhat cheating. Maybe ignore it.
   */
  bool grasped;

  /**
   * Only ever true for tools.
   */
  bool grasping;

  stId id;

  concuno::Float location[2];

  concuno::Float orientation;

  concuno::Float orientationVelocity;

  stType type;

  concuno::Float velocity[2];

};


struct stState {

  stState();

  bool cleared;

  concuno::List<stItem> items;

  double time;

};


void stItemInit(stItem* item);


/**
 * Inits the schema from scratch to have everything needed for stackiter data.
 *
 * Dispose with cnSchemaDispose (since nothing special needed here).
 */
void stSchemaInit(concuno::Schema* schema);


void stStateDispose(stState* state);


stItem* stStateFindItem(stState* state, stId id);


bool stStateCopy(stState* to, stState* from);


#endif
