#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h


#include <concuno.h>


namespace ccndomain {namespace stackiter {


typedef concuno::Index Id;


typedef int TypeId;


struct Item {

  enum Type {
    TypeNone,
    TypeBlock,
    TypeTool,
    TypeTray,
    TypeView,
    TypeWorld,
  };

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

  Id id;

  concuno::Float location[2];

  concuno::Float orientation;

  concuno::Float orientationVelocity;

  Type type;

  concuno::Float velocity[2];

};


struct State {

  State();

  bool cleared;

  concuno::List<Item> items;

  double time;

};


void stItemInit(Item* item);


/**
 * Inits the schema from scratch to have everything needed for stackiter data.
 *
 * Dispose with cnSchemaDispose (since nothing special needed here).
 */
void schemaInit(concuno::Schema& schema);


void stateDispose(State* state);


Item* stateFindItem(State* state, Id id);


bool stateCopy(State* to, State* from);


}}


#endif
