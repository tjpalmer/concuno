#ifndef stackiter_learner_h
#define stackiter_learner_h

#include <cuncuno.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace cuncuno;
using namespace std;

namespace stackiter {

enum ItemType {
  Block,
  Tool,
};

struct Item {
  int id;
  ItemType type;
};

struct State {
  MatrixXd angles;
  MatrixXd angularVelocities;
  MatrixXd colors;
  MatrixXd extents;
  MatrixXi ids;
  MatrixXd locations;
  Matrix<ItemType,Dynamic,Dynamic> types;
  MatrixXd velocities;
};

// See: EIGEN_DEFINE_STL_VECTOR_SPECIALIZATION
// Also: http://forum.kde.org/viewtopic.php?f=74&t=82894#p174194

struct Loader {

  void load(const string& name);

private:

  void handleAngle(stringstream* tokens);

  void handleAngularVelocity(stringstream* tokens);

  void handleColor(stringstream* tokens);

  void handleDestroy(stringstream* tokens);

  void handleExtent(stringstream* tokens);

  void handleItem(stringstream* tokens);

  void handleLocation(stringstream* tokens);

  void handleType(stringstream* tokens);

  void handleVelocity(stringstream* tokens);

  vector<int> indexes;

  vector<Item> items;

};

}

using namespace stackiter;

#endif
