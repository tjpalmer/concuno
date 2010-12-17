#include <cstring>
#include "entity2d3d.h"
#include "functions.h"

using namespace std;

namespace cuncuno {

Entity2D::Entity2D(): orientation(0), orientationVelocity(0) {
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(velocity, 0, sizeof(velocity));
}

const Type& Entity2D::type() {
  // TODO Figure out a better way to provide the type system or whatnot.
  static TypeSystem system;
  static Type entity2DType(system, "Entity2D", sizeof(Entity2D));
  Entity2D entity2D;
  static GetFunction location2DGet(
    "Location2D", entity2DType, system.$float().arrayType(2),
    // TODO Is there a better way to determine offsets?
    reinterpret_cast<Byte*>(&entity2D.location) -
    reinterpret_cast<Byte*>(&entity2D)
  );
  static PutFunction location2DPut(location2DGet);
  static bool first(true);
  if (first) {
    first = false;
    entity2DType.attributes.push_back(
      Attribute(&location2DGet, &location2DPut)
    );
  }
  return entity2DType;
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
