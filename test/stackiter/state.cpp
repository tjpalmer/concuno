#include "stackiter-learner.h"

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
  // TODO Other than setting the option, any way to zero with constructor?
  color.setZero();
  extent.setZero();
  location.setZero();
  velocity.setZero();
}

}
