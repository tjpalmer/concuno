#include <stddef.h>
#include "state.h"

using namespace concuno;


namespace ccndomain {namespace stackiter {


#define stFill(items, val, i, end) \
  end = items + sizeof(items)/sizeof(*items); \
  for (i = items; i < end; i++) { \
    *i = (val); \
  }


void stItemInit(Item* item) {
  Float *f, *end;
  item->alive = false;
  stFill(item->color, 0.0, f, end);
  stFill(item->extent, 0.0, f, end);
  item->grasped = false;
  item->grasping = false;
  item->id = 0; // TODO or -1?
  stFill(item->location, 0.0, f, end);
  item->orientation = 0.0;
  item->orientationVelocity = 0.0;
  item->type = Item::TypeNone;
  stFill(item->velocity, 0.0, f, end);
}


void schemaInit(Schema& schema) {
  // Item type.
  Type* type = new Type(schema, "Item", sizeof(Item));
  pushOrDelete(*schema.types, type);

  // Properties.
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Color", offsetof(Item, color), 3
  ));
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Location", offsetof(Item, location), 2
  ));
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Velocity", offsetof(Item, velocity), 2
  ));
}


State::State(): cleared(false), time(0) {}


bool stateCopy(State* to, State* from) {
  *to = *from;
  to->items.init();
  return cnListPushAll(&to->items, &from->items);
}


Item* stateFindItem(State* state, Id id) {
  cnListEachBegin(&state->items, Item, item) {
    if (item->id == id) {
      return item;
    }
  } cnEnd;
  return NULL;
}


}}
