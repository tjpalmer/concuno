#include "entity.h"


void cnBagDispose(cnBag* bag) {
  cnListDispose(&bag->entities);
  cnBagInit(bag);
}


void cnBagInit(cnBag* bag) {
  bag->label = cnFalse;
  cnListInit(&bag->entities, sizeof(void*));
}


void cnSchemaDispose(cnSchema* schema) {
  cnListEachBegin(&schema->types, cnType, type) {
    // TODO cnTypeDispose(type);
  } cnEnd;
  cnListDispose(&schema->types);
}


cnBool cnSchemaInitDefault(cnSchema* schema) {
  cnType *type, typeStorage;
  cnListInit(&schema->types, sizeof(cnType));
  if (!(type = cnListPush(&schema->types, &typeStorage))) {
    return cnFalse;
  }
  // TODO cnTypeInit(type, "Float", sizeof(cnFloat));
  return cnTrue;
}
