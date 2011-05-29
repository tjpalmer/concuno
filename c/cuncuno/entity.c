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
    cnTypeDispose(type);
  } cnEnd;
  cnListDispose(&schema->types);
}


cnBool cnSchemaInitDefault(cnSchema* schema) {
  cnType *type;
  cnListInit(&schema->types, sizeof(cnType));
  if (!(type = cnListExpand(&schema->types, 1))) {
    return cnFalse;
  }
  cnTypeInit(type, "Float", sizeof(cnFloat));
  return cnTrue;
}


void cnTypeDispose(cnType* type) {
  cnStringDispose(&type->name);
  cnListEachBegin(&type->properties, cnProperty, property) {
    // TODO cnPropertyDispose(property);
  } cnEnd;
  cnListDispose(&type->properties);
  type->size = 0;
  type->schema = NULL;
}


cnBool cnTypeInit(cnType* type, char* name, cnCount size) {
  cnStringInit(&type->name);
  if (!cnStringPushStr(&type->name, name)) {
    return cnFalse;
  }
  cnListInit(&type->properties, sizeof(cnProperty));
  type->size = size;
  // Let this be set later, if wanted.
  type->schema = NULL;
  return cnTrue;
}
