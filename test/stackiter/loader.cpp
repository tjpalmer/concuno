#include "stackiter-learner.h"

namespace stackiter {

void Loader::handleDestroy(stringstream* tokens) {
  int id;
  *tokens >> id;
  int index = indexes[id];
  if (index == -1) {
    throw "double destroy";
  }
  // Remove the destroyed item.
  indexes[id] = -1;
  items.erase(items.begin() + index);
  // Reduce the index of successive items.
  for (
    vector<Item>::iterator i = items.begin() + index; i < items.end(); i++
  ) {
    // TODO Could optimize further if we assume seeing items always in
    // TODO increasing order.
    const Item& item = *i;
    indexes[item.id]--;
  }
}

void Loader::handleItem(stringstream* tokens) {
  Item item;
  *tokens >> item.id;
  if (item.id < 0) {
    throw "negative item id";
  }
  // TODO Evil data copy here. Do I care?
  items.push_back(item);
  while (indexes.size() < static_cast<size_t>(item.id + 1)) {
    // TODO Or can I make an allocator creating -1 by default for resize?
    indexes.push_back(-1);
  }
  indexes[item.id] = items.size() - 1;
}

void Loader::handleType(stringstream* tokens) {
  int id;
  string type;
  *tokens >> id;
  *tokens >> type;
  // TODO Validate id.
  Item& item = items[indexes[id]];
  if (type == "block") {
    item.type = stackiter::Block;
  } else if (type == "tool") {
    item.type = Tool;
  }
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
    // TODO Make a map of handlers or something.
    if (command == "destroy") {
      handleDestroy(&tokens);
    } else if (command == "item") {
      handleItem(&tokens);
    } else if (command == "type") {
      handleType(&tokens);
    }
  }
  cout << "Items at end: " << items.size() << endl;
}

}
