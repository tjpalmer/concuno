#include <math.h>
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


void cnEntityFunctionCreateDifference_Get(
  cnEntityFunction* function, void** ins, void* outs
) {
  // TODO Remove float assumption here.
  cnIndex i;
  cnEntityFunction* base = function->data;
  // Vectors are assumed small, so use stack memory.
  // Our assumption that base->outCount == function->outCount allows the use
  // of base here. And the use of base here allows this difference function to
  // be used directly by distance as well.
  cnFloat* x = cnStackAlloc(base->outCount * sizeof(cnFloat));
  cnFloat* result = outs;
  if (!x) {
    // TODO Some way to report errors. Just NaN out for now.
    cnFloat nan = cnNaN();
    for (i = 0; i < base->outCount; i++) {
      result[i] = nan;
    }
    return;
  }
  base->get(base, ins, result);
  base->get(base, ins + 1, x);
  for (i = 0; i < base->outCount; i++) {
    result[i] -= x[i];
  }
  cnStackFree(x);
}


cnEntityFunction* cnEntityFunctionCreateDifference(
  const cnEntityFunction* base
) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = (void*)base;
  function->dispose = NULL;
  function->inCount = 2;
  cnStringInit(&function->name);
  function->outCount = base->outCount; // TODO For all topologies?
  function->outTopology = base->outTopology;
  // TODO Verify non-nulls?
  //printf("outType: %s\n", cnStr(&base->outType->name));
  if (base->outType != base->outType->schema->floatType) {
    // TODO Supply a difference function for generic handling?
    printf("Only works for floats so far.\n");
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnEntityFunctionDrop(function);
    return NULL;
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionCreateDifference_Get;
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Difference")) {
    cnEntityFunctionDrop(function);
    return NULL;
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnEntityFunctionDrop(function);
    return NULL;
  }
  return function;
}


void cnEntityFunctionCreateDistance_Get(
  cnEntityFunction* function, void** ins, void* outs
) {
  // TODO Remove float assumption here.
  cnIndex i;
  cnEntityFunction* base = function->data;
  // Vectors are assumed small, so use stack memory.
  cnFloat* diff = cnStackAlloc(base->outCount * sizeof(cnFloat));
  cnFloat* result = outs;
  if (!diff) {
    // TODO Some way to report errors. Just NaN out for now.
    *result = cnNaN();
    return;
  }
  // Get the difference. Reusing the current function object in the call below
  // is somewhat abusive, but it's been carefully tailored to be safe.
  cnEntityFunctionCreateDifference_Get(function, ins, diff);
  // Get the norm of the difference.
  *result = 0;
  for (i = 0; i < function->outCount; i++) {
    *result += diff[i] * diff[i];
  }
  // Using sqrt leaves results clearer and has a very small cost.
  *result = sqrt(*result);
  cnStackFree(diff);
}

cnEntityFunction* cnEntityFunctionCreateDistance(const cnEntityFunction* base) {
  // TODO Combine setup with difference? Differences indicated below.
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = (void*)base;
  function->dispose = NULL;
  function->inCount = 2;
  cnStringInit(&function->name);
  function->outCount = 1; // Different from difference!
  function->outTopology = base->outTopology;
  // TODO Verify non-nulls?
  //printf("outType: %s\n", cnStr(&base->outType->name));
  if (base->outType != base->outType->schema->floatType) {
    // TODO Supply a difference function for generic handling?
    printf("Only works for floats so far.\n");
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnEntityFunctionDrop(function);
    return NULL;
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionCreateDistance_Get; // Also different!
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Distance")) { // Also different!
    cnEntityFunctionDrop(function);
    return NULL;
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnEntityFunctionDrop(function);
    return NULL;
  }
  return function;
}


void cnEntityFunctionCreateProperty_Get(
  cnEntityFunction* function, void** ins, void* outs
) {
  cnProperty* property = function->data;
  if (!*ins) {
    // Provide NaNs for floats when no input given.
    // TODO Anything for other types? Error result?
    if (function->outType == function->outType->schema->floatType) {
      cnFloat nan = cnNaN();
      cnIndex o;
      for (o = 0; o < function->outCount; o++) {
        ((cnFloat*)outs)[o] = nan;
      }
    }
  } else {
    property->get(property, *ins, outs);
  }
}

cnEntityFunction* cnEntityFunctionCreateProperty(const cnProperty* property) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = (void*)property; // Treat as const anyway!
  function->dispose = NULL;
  function->inCount = 1;
  function->outCount = property->count;
  function->outTopology = property->topology;
  function->outType = property->type;
  function->get = cnEntityFunctionCreateProperty_Get;
  cnStringInit(&function->name);
  // The one thing that can fail directly here.
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&property->name))) {
    cnEntityFunctionDrop(function);
    return NULL;
  }
  return function;
}


void cnEntityFunctionDrop(cnEntityFunction* function) {
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
  free(function);
}


void cnFunctionDrop(cnFunction* function) {
  if (function) {
    if (function->dispose) {
      function->dispose(function);
    }
    free(function);
  }
}


void cnPredicateDrop(cnPredicate* predicate) {
  if (predicate) {
    if (predicate->dispose) {
      predicate->dispose(predicate);
    }
    free(predicate);
  }
}


typedef struct {
  cnFunction* distanceFunction;
  cnFloat threshold;
} cnPredicateCreateDistanceThreshold_Data;

void cnPredicateCreateDistanceThreshold_Dispose(cnPredicate* predicate) {
  cnPredicateCreateDistanceThreshold_Data* data = predicate->data;
  cnFunctionDrop(data->distanceFunction);
  free(data);
  predicate->data = NULL;
  predicate->dispose = NULL;
  predicate->evaluate = NULL;
}

cnBool cnPredicateCreateDistanceThreshold_Evaluate(
  cnPredicate* predicate, void* in
) {
  cnPredicateCreateDistanceThreshold_Data* data = predicate->data;
  cnFloat distance;
  if (
    !data->distanceFunction->evaluate(data->distanceFunction, in, &distance)
  ) {
    // TODO Something better than this abuse.
    return -1;
  }
  // TODO I'd prefer <, but need better a handling of bulks of equal distances
  // TODO in threshold choosing. I've hit the problem before.
  return distance <= data->threshold;
}

cnPredicate* cnPredicateCreateDistanceThreshold(
  cnFunction* distanceFunction, cnFloat threshold
) {
  cnPredicate* predicate = malloc(sizeof(cnPredicate));
  cnPredicateCreateDistanceThreshold_Data* data;
  if (!predicate) {
    return NULL;
  }
  data = malloc(sizeof(cnPredicateCreateDistanceThreshold_Data));
  if (!data) {
    free(predicate);
    return NULL;
  }
  // Good to go.
  data->distanceFunction = distanceFunction;
  data->threshold = threshold;
  predicate->data = data;
  predicate->dispose = cnPredicateCreateDistanceThreshold_Dispose;
  predicate->evaluate = cnPredicateCreateDistanceThreshold_Evaluate;
  return predicate;
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
