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
  cnType *type;
  if (!cnSchemaInitDefault(schema)) {
    return cnFalse;
  }
  if (!(type = cnListExpand(&schema->types))) {
    return cnFalse;
  }
  // TODO Init item type.
  return cnTrue;
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
