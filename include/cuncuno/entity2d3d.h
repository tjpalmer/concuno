#ifndef cuncuno_entity2d3d_h
#define cuncuno_entity2d3d_h

#include "entity.h"

namespace cuncuno {

/**
 * An easy struct for basic 2D physical entities. Can avoid the need for
 * designing your own schema.
 */
struct Entity2D {

  /**
   * The standard type for Entity2D. Can be copied and expanded as needed.
   */
  static const Type& type();

  Entity2D();

  Float color[3];

  /**
   * Half sizes, or radii in a sense.
   */
  Float extent[2];

  Float location[2];

  /**
   * Just the angle of rotation.
   */
  Float orientation;

  Float orientationVelocity;

  Float velocity[2];

};

/**
 * An easy struct for basic 3D physical entities. Can avoid the need for
 * designing your own schema.
 */
struct Entity3D {

  Entity3D();

  Float color[3];

  /**
   * Half sizes, or radii in a sense.
   */
  Float extent[3];

  Float location[3];

  /**
   * As a quaternion, or use axis-angle? If axis-angle, normalize to 3 vals?
   */
  Float orientation[4];

  /**
   * As a quaternion, or use axis-angle? If axis-angle, normalize to 3 vals?
   */
  Float orientationVelocity[4];

  Float velocity[3];

};

}

#endif
