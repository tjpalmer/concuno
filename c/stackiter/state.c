#include "state.h"


#define stFill(items, val, i, end) \
  end = items + sizeof(items)/sizeof(*items); \
  for (i = items; i < end; i++) { \
    *i = (val); \
  }


void stItemInit(stItem* item) {
  cnFloat *f, *end;
  item->alive = cnFalse;
  stFill(item->color, 0.0, f, end);
  item->grasped = cnFalse;
  item->grasping = cnFalse;
  item->id = 0; // TODO or -1?
  stFill(item->location, 0.0, f, end);
  item->typeId = stTypeNone;
}


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
