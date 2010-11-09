#include <cstring>
#include "state.h"

using namespace std;

namespace stackiter {

Item::Item():
  alive(false),
  angle(0),
  angularVelocity(0),
  grasped(false),
  grasping(false),
  id(-1),
  type(None)
{
  // TODO Any way to zero with constructor?
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(velocity, 0, sizeof(velocity));
}

State::State(): cleared(false) {}

const Item* State::findItem(int id) const {
  for (
    vector<Item>::const_iterator i = items.begin(); i != items.end(); i++
  ) {
    const Item& item = *i;
    if (item.id == id) {
      return &item;
    }
  }
  return 0;
}

}
