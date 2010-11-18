#include <cstring>
#include "entity2d3d.h"

using namespace std;

namespace cuncuno {

struct Location2DAttribute: Attribute {
  Location2DAttribute(): Attribute("Location2D", Type::$float(), 2) {}
  virtual void get(const void* entity, void* buffer) const {
    const Entity2D& e2d = *reinterpret_cast<const Entity2D*>(entity);
    memcpy(buffer, e2d.location, sizeof(e2d.location));
  }
};

Entity2D::Entity2D(): orientation(0), orientationVelocity(0) {
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(velocity, 0, sizeof(velocity));
}

const Type& Entity2D::type() {
  static Type entity2DType;
  entity2DType.name = "Entity2D";
  entity2DType.size = sizeof(Entity2D);
  entity2DType.attributes.push_back(new Location2DAttribute);
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
