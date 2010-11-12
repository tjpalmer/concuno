#include <cstring>
#include "entity2d3d.h"

using namespace std;

namespace cuncuno {

struct Location2DAttribute: FloatAttribute {
  virtual size_t count() const {
    return 2;
  }
  virtual void get(const Entity* entity, Float* values) const {
    const Entity2D* e2d = reinterpret_cast<const Entity2D*>(entity);
    memcpy(values, e2d->location, sizeof(e2d->location));
  }
  virtual void name(string& buffer) const {
    buffer = "Location2D";
  }
};

Entity2D::Entity2D(): orientation(0), orientationVelocity(0) {
  memset(color, 0, sizeof(color));
  memset(extent, 0, sizeof(extent));
  memset(location, 0, sizeof(location));
  memset(velocity, 0, sizeof(velocity));
}

void Entity2D::pushSchema(Schema& schema) {
  schema.floatAttributes.push_back(new Location2DAttribute);
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
