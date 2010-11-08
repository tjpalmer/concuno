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
typedef void(Loader::*LoaderHandler)(std::stringstream& tokens);

struct Loader {

  Loader();

  void load(const std::string& name);

  /**
   * The full list of states in the file.
   *
   * TODO Should this be a pointer, optionally passed in, so it can outlast
   * TODO the loader?
   */
  std::vector<State> states;

private:

  Item& getItem(std::stringstream& tokens);

  void handleAlive(std::stringstream& tokens);

  void handleAngle(std::stringstream& tokens);

  void handleAngularVelocity(std::stringstream& tokens);

  void handleColor(std::stringstream& tokens);

  void handleDestroy(std::stringstream& tokens);

  void handleExtent(std::stringstream& tokens);

  void handleGrasp(std::stringstream& tokens);

  static int handleId(std::stringstream& tokens);

  void handleItem(std::stringstream& tokens);

  void handleLocation(std::stringstream& tokens);

  void handleRelease(std::stringstream& tokens);

  void handleTime(std::stringstream& tokens);

  void handleType(std::stringstream& tokens);

  void handleVelocity(std::stringstream& tokens);

  void pushState();

  std::vector<int> indexes;

  /**
   * TODO Some way to make this static? Survive instance for now.
   */
  std::map<std::string, LoaderHandler> handlers;

  State state;

};

}

#endif
