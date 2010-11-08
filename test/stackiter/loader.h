#ifndef stackiter_learner_loader_h
#define stackiter_learner_loader_h

#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "state.h"

namespace stackiter {

struct Loader;
typedef void(Loader::*LoaderHandler)(stringstream& tokens);

struct Loader {

  Loader();

  void load(const string& name);

  /**
   * The full list of states in the file.
   *
   * TODO Should this be a pointer, optionally passed in, so it can outlast
   * TODO the loader?
   */
  vector<State> states;

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

  void pushState();

  vector<int> indexes;

  /**
   * TODO Some way to make this static? Survive instance for now.
   */
  map<string, LoaderHandler> handlers;

  State state;

};

}

#endif
