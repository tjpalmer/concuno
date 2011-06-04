#include <stdio.h>
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


void cnEntityFunctionDispose(cnEntityFunction* function) {
  if (function->dispose) {
    function->dispose(function);
    function->dispose = NULL;
  }
  cnStringDispose(&function->name);
  function->data = NULL;
  function->get = NULL;
  function->outTopology = cnTopologyEuclidean;
  function->outCount = 0;
  function->outType = NULL;
}


void cnEntityFunctionGetDifference(
  const cnEntityFunction* function, const void *const *ins, void* outs
) {
  // TODO Remove float assumption here.
  cnIndex i;
  cnEntityFunction* base = function->data;
  cnFloat* x = malloc(function->outCount * sizeof(cnFloat));
  cnFloat* result = outs;
  if (!x) {
    // TODO Some way to report errors. Just zero out for now.
    for (i = 0; i < function->outCount; i++) {
      result[i] = 0;
    }
    return;
  }
  base->get(base, ins, result);
  base->get(base, ins + 1, x);
  for (i = 0; i < function->outCount; i++) {
    result[i] -= x[i];
  }
  free(x);
}


void cnEntityFunctionGetProperty(
  const cnEntityFunction* function, const void *const *ins, void* outs
) {
  cnProperty* property = function->data;
  property->get(property, *ins, outs);
}


cnBool cnEntityFunctionInitDifference(
  cnEntityFunction* function, const cnEntityFunction* base
) {
  function->data = (void*)base;
  function->dispose = NULL;
  function->inCount = 2;
  cnStringInit(&function->name);
  function->outCount = base->outCount;
  function->outTopology = base->outTopology;
  // TODO Verify non-nulls?
  //printf("outType: %s\n", cnStr(&base->outType->name));
  if (base->outType != base->outType->schema->floatType) {
    // TODO Supply a difference function for generic handling?
    printf("Only works for floats so far.\n");
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnEntityFunctionDispose(function);
    return cnFalse;
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionGetDifference;
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Difference")) {
    cnEntityFunctionDispose(function);
    return cnFalse;
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnEntityFunctionDispose(function);
    return cnFalse;
  }
  return cnTrue;
}


cnBool cnEntityFunctionInitProperty(
  cnEntityFunction* function, const cnProperty* property
) {
  function->data = (void*)property; // Treat as const anyway!
  function->dispose = NULL;
  function->inCount = 1;
  function->outCount = property->count;
  function->outTopology = property->topology;
  function->outType = property->type;
  function->get = cnEntityFunctionGetProperty;
  cnStringInit(&function->name);
  // The one thing that can fail directly here.
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&property->name))) {
    cnEntityFunctionDispose(function);
    return cnFalse;
  }
  return cnTrue;
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
  property->topology = cnTopologyEuclidean;
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
  cnProperty* property, cnType* containerType, cnType* type, char* name,
  cnCount offset, cnCount count
) {
  // Safety items first.
  cnStringInit(&property->name);
  property->dispose = NULL;
  // Now other things.
  if (!cnStringPushStr(&property->name, name)) {
    cnPropertyDispose(property);
    return cnFalse;
  }
  property->containerType = containerType;
  property->count = count;
  property->get = cnPropertyFieldGet;
  property->offset = offset;
  property->put = cnPropertyFieldPut;
  property->topology = cnTopologyEuclidean;
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
  if (!(type = cnListExpand(&schema->types))) {
    cnSchemaDispose(schema);
    return cnFalse;
  }
  if (!cnTypeInit(type, "Float", sizeof(cnFloat))) {
    cnSchemaDispose(schema);
    return cnFalse;
  }
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
  // Put safety values first.
  type->size = size;
  // Let schema be set later, if wanted.
  type->schema = NULL;
  cnStringInit(&type->name);
  cnListInit(&type->properties, sizeof(cnProperty));
  // Now try things that might fail.
  if (!cnStringPushStr(&type->name, name)) {
    return cnFalse;
  }
  return cnTrue;
}
