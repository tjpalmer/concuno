#include <stddef.h>
#include "state.h"

using namespace concuno;


#define stFill(items, val, i, end) \
  end = items + sizeof(items)/sizeof(*items); \
  for (i = items; i < end; i++) { \
    *i = (val); \
  }


void stItemInit(stItem* item) {
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
  item->type = stTypeNone;
  stFill(item->velocity, 0.0, f, end);
}


void stSchemaInit(Schema& schema) {
  // Item type.
  Type* type = new Type(schema, "Item", sizeof(stItem));
  pushOrDelete(*schema.types, type);

  // Properties.
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Color", offsetof(stItem, color), 3
  ));
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Location", offsetof(stItem, location), 2
  ));
  type->properties.push(new OffsetProperty(
    type, schema.floatType, "Velocity", offsetof(stItem, velocity), 2
  ));
}


stState::stState(): cleared(false), time(0) {}


bool stStateCopy(stState* to, stState* from) {
  *to = *from;
  to->items.init();
  return cnListPushAll(&to->items, &from->items);
}


stItem* stStateFindItem(stState* state, stId id) {
  cnListEachBegin(&state->items, stItem, item) {
    if (item->id == id) {
      return item;
    }
  } cnEnd;
  return NULL;
}
