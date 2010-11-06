#ifndef stackiter_learner_h
#define stackiter_learner_h

#include <cuncuno.h>
#include <Eigen/StdVector>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>

using namespace cuncuno;
using namespace std;

namespace stackiter {

enum ItemType {
  Block,
  Tool,
  Tray,
  View,
  World,
};

struct Item {

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

struct Loader;
typedef void(Loader::*LoaderHandler)(stringstream& tokens);

struct Loader {

  Loader();

  void load(const string& name);

private:

  Item& getItem(stringstream& tokens);

  void handleAlive(stringstream& tokens);

  void handleAngle(stringstream& tokens);

  void handleAngularVelocity(stringstream& tokens);

  void handleColor(stringstream& tokens);

  void handleDestroy(stringstream& tokens);

  void handleExtent(stringstream& tokens);

  void handleGrasp(stringstream& tokens);

  static int handleId(stringstream& tokens);

  void handleItem(stringstream& tokens);

  void handleLocation(stringstream& tokens);

  void handleRelease(stringstream& tokens);

  void handleTime(stringstream& tokens);

  void handleType(stringstream& tokens);

  void handleVelocity(stringstream& tokens);

  vector<int> indexes;

  /**
   * TODO Some way to make this static? Survive instance for now.
   */
  map<string, LoaderHandler> handlers;

  State state;

};

}

using namespace stackiter;

#endif
