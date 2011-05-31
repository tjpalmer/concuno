#include <string.h>
#include "entity.h"


void cnBagDispose(cnBag* bag) {
  cnListDispose(&bag->entities);
  cnBagInit(bag);
}


void cnBagInit(cnBag* bag) {
  bag->label = cnFalse;
  cnListInit(&bag->entities, sizeof(void*));
}


void cnPropertyDispose(cnProperty* property) {
  // Dispose of extra data, as needed.
  if (property->dispose) {
    property->dispose(property);
    property->dispose = NULL;
  }
  // Also 0 out the offset. As a union, this might be unneeded and/or
  // insufficient, but here goes.
  property->offset = 0;
  property->data = NULL;
  // Clear out the name.
  cnStringDispose(&property->name);
  // Clear out the simple things.
  property->count = 0;
  property->get = NULL;
  property->put = NULL;
  property->type = NULL;
}

void cnPropertyFieldGet(
  const cnProperty* property, const void* entity, void* storage
) {
  memcpy(
    storage,
    ((char*)entity) + property->offset,
    property->count * property->type->size
  );
}

void cnPropertyFieldPut(
  const cnProperty* property, void* entity, const void* value
) {
  memcpy(
    ((char*)entity) + property->offset,
    value,
    property->count * property->type->size
  );
}

cnBool cnPropertyInitField(
  cnProperty* property, cnType* type, char* name, cnCount offset, cnCount count
) {
  cnStringInit(&property->name);
  // TODO Preset all to nullishness, in case push fails?
  if (!cnStringPushStr(&property->name, name)) {
    return cnFalse;
  }
  property->count = count;
  property->dispose = NULL;
  property->get = cnPropertyFieldGet;
  property->offset = offset;
  property->put = cnPropertyFieldPut;
  property->type = type;
  return cnTrue;
}

void cnSchemaDispose(cnSchema* schema) {
  cnListEachBegin(&schema->types, cnType, type) {
    cnTypeDispose(type);
  } cnEnd;
  cnListDispose(&schema->types);
  schema->floatType = NULL;
}


cnBool cnSchemaInitDefault(cnSchema* schema) {
  cnType *type;
  cnListInit(&schema->types, sizeof(cnType));
  if (!(type = cnListExpand(&schema->types, 1))) {
    return cnFalse;
  }
  cnTypeInit(type, "Float", sizeof(cnFloat));
  type->schema = schema;
  schema->floatType = type;
  return cnTrue;
}


void cnTypeDispose(cnType* type) {
  cnStringDispose(&type->name);
  cnListEachBegin(&type->properties, cnProperty, property) {
    cnPropertyDispose(property);
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
