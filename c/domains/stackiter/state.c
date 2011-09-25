#include <stddef.h>
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
  stFill(item->extent, 0.0, f, end);
  item->grasped = cnFalse;
  item->grasping = cnFalse;
  item->id = 0; // TODO or -1?
  stFill(item->location, 0.0, f, end);
  item->orientation = 0.0;
  item->orientationVelocity = 0.0;
  item->type = stTypeNone;
  stFill(item->velocity, 0.0, f, end);
}


cnBool stSchemaInit(cnSchema* schema) {
  cnProperty* property;
  cnType* type;

  if (!cnSchemaInitDefault(schema)) {
    return cnFalse;
  }

  // Item type.
  if (!(type = cnTypeCreate("Item", sizeof(stItem)))) goto FAIL;
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) goto FAIL;

  // Color property.
  if (!(property = cnListExpand(&type->properties))) goto FAIL;
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Color",
    offsetof(stItem, color), 3
  )) goto FAIL;

  // Location property.
  if (!(property = cnListExpand(&type->properties))) goto FAIL;
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Location",
    offsetof(stItem, location), 2
  )) goto FAIL;

  // Velocity property.
  if (!(property = cnListExpand(&type->properties))) goto FAIL;
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Velocity",
    offsetof(stItem, velocity), 2
  )) goto FAIL;

  // Good to go.
  return cnTrue;

  FAIL:
  cnSchemaDispose(schema);
  return cnFalse;
}


cnBool stStateCopy(stState* to, stState* from) {
  *to = *from;
  cnListInit(&to->items, to->items.itemSize);
  return cnListPushAll(&to->items, &from->items) != NULL;
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


stItem* stStateFindItem(stState* state, stId id) {
  cnListEachBegin(&state->items, stItem, item) {
    if (item->id == id) {
      return item;
    }
  } cnEnd;
  return NULL;
}