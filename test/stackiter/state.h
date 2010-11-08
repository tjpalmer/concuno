#ifndef stackiter_learner_state_h
#define stackiter_learner_state_h

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <vector>

using namespace Eigen;
using namespace std;

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

  Vector3d color;

  Vector2d extent;

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

  Vector2d location;

  ItemType type;

  Vector2d velocity;

  // To keep Eigen alignment happy.
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

};

}

/**
 * Have to define this outside namespace stackiter but before Item is used for
 * vectors.
 */
EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION(::stackiter::Item)

// Back to your regularly scheduled namespace ...

namespace stackiter {

struct State {

  vector<Item> items;

  /**
   * Simulation time, actually.
   *
   * TODO Break this out as a separately typed item?
   */
  double time;

};

// Fully arrayed version.
// TODO How do I want to expose entity attributes to cuncuno?
//struct State {
//  MatrixXd angles;
//  MatrixXd angularVelocities;
//  MatrixXd colors;
//  MatrixXd extents;
//  MatrixXi ids;
//  MatrixXd locations;
//  Matrix<ItemType,Dynamic,Dynamic> types;
//  MatrixXd velocities;
//};

// See: EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION
// Also: http://forum.kde.org/viewtopic.php?f=74&t=82894#p174194

}

#endif
