#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h

#include <cuncuno.h>
#include <vector>

namespace stackiter {

enum ItemType {
  None,
  Block,
  Tool,
  Tray,
  View,
  World,
};

struct Item: cuncuno::Entity2D {

  Item();

  bool alive;

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

};

struct State {

  State();

  const Item* findItem(int id) const;

  bool cleared;

  std::vector<Item> items;

  /**
   * Simulation time, actually.
   *
   * TODO Break this out as a separately typed item?
   */
  double time;

};

}

#endif
