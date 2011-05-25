#include "state.h"


void stStateDispose(stState* state) {
  cnListDispose(&state->items);
  stStateInit(state);
}


void stStateInit(stState* state) {
  state->cleared = cnFalse;
  cnListInit(&state->items, sizeof(stItem));
  state->time = 0;
}


/*

using namespace std;

namespace stackiter {

Item::Item(): alive(false), grasped(false), grasping(false), id(0), typeId(0) {}

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

*/
