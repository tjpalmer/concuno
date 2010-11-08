#include <cstring>
#include "state.h"

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

}
