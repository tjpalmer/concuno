#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h

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

struct Item {

  Item();

  bool alive;

  double angle;

  double angularVelocity;

  double color[3];

  double extent[2];

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

  int id;

  double location[2];

  ItemType type;

  double velocity[2];

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
