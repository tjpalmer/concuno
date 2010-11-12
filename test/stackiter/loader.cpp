#include "loader.h"

using namespace std;

namespace stackiter {

Loader::Loader() {
  handlers["alive"] = &Loader::handleAlive;
  handlers["clear"] = &Loader::handleClear;
  handlers["color"] = &Loader::handleColor;
  handlers["destroy"] = &Loader::handleDestroy;
  handlers["extent"] = &Loader::handleExtent;
  handlers["grasp"] = &Loader::handleGrasp;
  handlers["item"] = &Loader::handleItem;
  handlers["pos"] = &Loader::handleLocation;
  handlers["posvel"] = &Loader::handleVelocity;
  handlers["release"] = &Loader::handleRelease;
  handlers["rot"] = &Loader::handleOrientation;
  handlers["rotvel"] = &Loader::handleOrientationVelocity;
  handlers["time"] = &Loader::handleTime;
  handlers["type"] = &Loader::handleType;
}

Item& Loader::getItem(stringstream& tokens) {
  // TODO Any validation?
  return state.items[indexes[handleId(tokens)]];
}

void Loader::handleAlive(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Does this really work from strings?
  string alive;
  tokens >> alive;
  item.alive = alive == "true";
}

void Loader::handleClear(stringstream& tokens) {
  state.cleared = true;
}

void Loader::handleColor(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Use HSV colorspace to begin with?
  // TODO Verify we haven't run out?
  tokens >> item.color[0];
  tokens >> item.color[1];
  tokens >> item.color[2];
  // Ignore opacity, the 4th value. It's bogus for now.
}

void Loader::handleDestroy(stringstream& tokens) {
  int id = handleId(tokens);
  int index = indexes[id];
  if (!index) {
    throw "double destroy";
  }
  // Remove the destroyed item.
  indexes[id] = 0;
  state.items.erase(state.items.begin() + index);
  // Reduce the index of successive items.
  for (
    vector<Item>::iterator i = state.items.begin() + index;
    i < state.items.end();
    i++
  ) {
    // TODO Could optimize further if we assume seeing items always in
    // TODO increasing order.
    const Item& item = *i;
    indexes[item.id]--;
  }
}

void Loader::handleExtent(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.extent[0];
  tokens >> item.extent[1];
}

int Loader::handleId(stringstream& tokens) {
  int id;
  tokens >> id;
  if (id <= 0) {
    throw "nonpositive id";
  }
  return id;
}

void Loader::handleGrasp(stringstream& tokens) {
  Item& tool = getItem(tokens);
  Item& item = getItem(tokens);
  // TODO Parse and use the relative grasp location?
  tool.grasping = true;
  item.grasped = true;
}

void Loader::handleItem(stringstream& tokens) {
  Item item;
  item.id = handleId(tokens);
  // TODO Verify against duplicate ID?
  // TODO Evil data copy here. Do I care?
  state.items.push_back(item);
  while (indexes.size() < static_cast<size_t>(item.id + 1)) {
    // TODO Or is it 0 by default for resize?
    indexes.push_back(0);
  }
  indexes[item.id] = state.items.size() - 1;
}

void Loader::handleLocation(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.location[0];
  tokens >> item.location[1];
}

void Loader::handleOrientation(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Angle is in rats. Convert to radians or not?
  tokens >> item.orientation;
}

void Loader::handleOrientationVelocity(stringstream& tokens) {
  Item& item = getItem(tokens);
  // TODO Angular velocity is in rats. Convert to radians or not?
  tokens >> item.orientationVelocity;
}

void Loader::handleRelease(stringstream& tokens) {
  Item& tool = getItem(tokens);
  Item& item = getItem(tokens);
  tool.grasping = false;
  item.grasped = false;
}

void Loader::handleTime(stringstream& tokens) {
  string type;
  tokens >> type;
  if (type == "sim") {
    pushState();
    // TODO Some general 'reset' function?
    state.cleared = false;
    // Just eat the number of steps for now. Maybe I'll care more about it
    // later.
    int steps;
    tokens >> steps;
    // I pretend the sim time (in seconds) is what matters here.
    tokens >> state.time;
  }
}

void Loader::handleType(stringstream& tokens) {
  Item& item = getItem(tokens);
  string type;
  tokens >> type;
  // TODO Validate id.
  if (type == "block") {
    item.typeId = stackiter::Block;
  } else if (type == "tool") {
    item.typeId = Tool;
  }
}

void Loader::handleVelocity(stringstream& tokens) {
  Item& item = getItem(tokens);
  tokens >> item.velocity[0];
  tokens >> item.velocity[1];
}

void Loader::load(const string& name) {
  cout << "Loading " << name << endl;
  ifstream in(name.c_str());
  if (!in.is_open()) {
    throw "failed to open";
  }
  string command;
  string line;
  while (!getline(in, line).eof()) {
    stringstream tokens(line);
    tokens >> command;
    map<string,LoaderHandler>::iterator h = handlers.find(command);
    if (h != handlers.end()) {
      // We've defined a handler for this command, so handle it.
      LoaderHandler handler = h->second;
      (this->*handler)(tokens);
    }
  }
  // Record the end state.
  pushState();
}

void Loader::pushState() {
  // Save a copy of the current state.
  states.push_back(state);
}

}
