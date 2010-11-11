#include <cstring>
#include "state.h"

using namespace std;

namespace stackiter {

Item::Item(): alive(false), grasped(false), grasping(false) {}

State::State(): cleared(false) {}

const Item* State::findItem(int id) const {
  for (
    vector<Item>::const_iterator i = items.begin(); i != items.end(); i++
  ) {
    const Item& item = *i;
    if (item.id == id) {
      return &item;
    }
  }
  return 0;
}

}
