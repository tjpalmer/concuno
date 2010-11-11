#include <cstring>
#include "cuncuno.h"

using namespace std;

namespace cuncuno {

Entity::Entity(): id(0), type(0) {}

Entity2D::Entity2D(): orientation(0), orientationVelocity(0) {
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(velocity, 0, sizeof(velocity));
}

Entity3D::Entity3D() {
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(orientation, 0, sizeof(orientation));
  memset(orientationVelocity, 0, sizeof(orientationVelocity));
  memset(velocity, 0, sizeof(velocity));
}

}
