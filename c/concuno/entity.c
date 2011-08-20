#include <math.h>
#include <string.h>

#include "entity.h"
#include "io.h"


void cnBagDispose(cnBag* bag) {
  // If managed elsewhere, the list might be nulled, so check it.
  if (bag->entities) {
    cnListDispose(bag->entities);
    free(bag->entities);
  }

  // Participants are always owned by the bag, however.
  // TODO Really? By allowing outside control, but it be made more efficient?
  cnListEachBegin(&bag->participantOptions, cnList(cnEntity), list) {
    cnListDispose(list);
  } cnEnd;
  cnListDispose(&bag->participantOptions);

  // Other stuff.
  bag->label = cnFalse;
  bag->entities = NULL;
}


cnBool cnBagInit(cnBag* bag, cnList(cnEntity)* entities) {
  cnBool result = cnFalse;

  // Safety first.
  bag->label = cnFalse;
  bag->entities = entities ? entities : malloc(sizeof(cnList(cnEntity)));
  cnListInit(&bag->participantOptions, sizeof(cnList(cnEntity)));

  // Check on and finish up entities.
  if (!bag->entities) cnFailTo(DONE, "No bag entities.");
  if (!entities) cnListInit(bag->entities, sizeof(cnEntity));

  // Winned!
  result = cnTrue;

  DONE:
  return result;
}


void cnEntityFunctionCreateDifference_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
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


cnEntityFunction* cnEntityFunctionCreateDifference(cnEntityFunction* base) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = base;
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
  function->get = cnEntityFunctionCreateDifference_get;
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


void cnEntityFunctionCreateDistance_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
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
  cnEntityFunctionCreateDifference_get(function, ins, diff);
  // Get the norm of the difference.
  *result = 0;
  for (i = 0; i < function->outCount; i++) {
    *result += diff[i] * diff[i];
  }
  // Using sqrt leaves results clearer and has a very small cost.
  *result = sqrt(*result);
  cnStackFree(diff);
}

cnEntityFunction* cnEntityFunctionCreateDistance(cnEntityFunction* base) {
  // TODO Combine setup with difference? Differences indicated below.
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = base;
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
  function->get = cnEntityFunctionCreateDistance_get; // Also different!
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


void cnEntityFunctionCreateProperty_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
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

cnEntityFunction* cnEntityFunctionCreateProperty(cnProperty* property) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = property;
  function->dispose = NULL;
  function->inCount = 1;
  function->outCount = property->count;
  function->outTopology = property->topology;
  function->outType = property->type;
  function->get = cnEntityFunctionCreateProperty_get;
  cnStringInit(&function->name);
  // The one thing that can fail directly here.
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&property->name))) {
    cnEntityFunctionDrop(function);
    return NULL;
  }
  return function;
}


void cnEntityFunctionCreateValid_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  cnIndex i;
  cnFloat* out = outs;
  for (i = 0; i < function->inCount; i++) {
    if (!ins[i]) {
      // No good, so false.
      // TODO Something other than float output.
      // TODO Meanwhile, is -1.0 or 0.0 better? Does it matter?
      *out = 0.0;
      return;
    }
  }
  // All clear.
  *out = 1.0;
}

cnEntityFunction* cnEntityFunctionCreateValid(cnSchema* schema, cnCount arity) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) return NULL;
  function->data = NULL;
  function->dispose = NULL;
  function->inCount = arity;
  function->outCount = 1;
  function->outTopology = cnTopologyEuclidean; // TODO Discrete or ordinal?
  function->outType = schema->floatType; // TODO Integer or some such?
  function->get = cnEntityFunctionCreateValid_get;
  cnStringInit(&function->name);
  // The one thing that can fail directly here.
  // TODO Customize name by arity?
  if (!cnStringPushStr(&function->name, "Valid")) {
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


cnEntityFunction* cnEntityFunctionCreate(
  char* name, cnCount inCount, cnCount outCount
) {
  cnEntityFunction* function = malloc(sizeof(cnEntityFunction));
  if (!function) cnFailTo(DONE, "No function.");
  function->data = NULL;
  function->dispose = NULL;
  function->inCount = inCount;
  function->outCount = outCount;
  function->outTopology = cnTopologyEuclidean;
  function->outType = NULL;
  function->get = NULL;
  cnStringInit(&function->name);
  // Build name now.
  if (!cnStringPushStr(&function->name, name)) {
    cnEntityFunctionDrop(function);
    function = NULL;
    cnFailTo(DONE, "No function name.");
  }
  DONE:
  return function;
}


cnFunction* cnFunctionCopy(cnFunction* function) {
  return function ? function->copy(function) : NULL;
}


void cnFunctionDrop(cnFunction* function) {
  if (function) {
    if (function->dispose) {
      function->dispose(function);
    }
    free(function);
  }
}


cnBool cnFunctionWrite(cnFunction* function, FILE* file, cnString* indent) {
  return function->write ? function->write(function, file, indent) : cnFalse;
}


/**
 * A helper for various composite entity functions.
 */
cnEntityFunction* cnPushCompositeFunction(
  cnList(cnEntityFunction*)* functions,
  cnEntityFunction* (*wrapper)(cnEntityFunction* base),
  cnEntityFunction* base
) {
  cnEntityFunction* function;
  if ((function = wrapper(base))) {
    // So far, so good.
    if (!cnListPush(functions, &function)) {
      // Nope, we failed.
      cnEntityFunctionDrop(function);
      function = NULL;
    }
  }
  return function;
}


cnEntityFunction* cnPushDifferenceFunction(
  cnList(cnEntityFunction*)* functions, cnEntityFunction* base
) {
  return
    cnPushCompositeFunction(functions, cnEntityFunctionCreateDifference, base);
}


cnEntityFunction* cnPushDistanceFunction(
  cnList(cnEntityFunction*)* functions, cnEntityFunction* base
) {
  return
    cnPushCompositeFunction(functions, cnEntityFunctionCreateDistance, base);
}


cnEntityFunction* cnPushPropertyFunction(
  cnList(cnEntityFunction*)* functions, cnProperty* property
) {
  cnEntityFunction* function;
  if ((function = cnEntityFunctionCreateProperty(property))) {
    // So far, so good.
    if (!cnListPush(functions, &function)) {
      // Nope, we failed.
      cnEntityFunctionDrop(function);
      function = NULL;
    }
  }
  return function;
}


cnEntityFunction* cnPushValidFunction(
  cnList(cnEntityFunction*)* functions, cnSchema* schema, cnCount arity
) {
  cnEntityFunction* function;
  if ((function = cnEntityFunctionCreateValid(schema, arity))) {
    // So far, so good.
    if (!cnListPush(functions, &function)) {
      // Nope, we failed.
      cnEntityFunctionDrop(function);
      function = NULL;
    }
  }
  return function;
}


cnPredicate* cnPredicateCopy(cnPredicate* predicate) {
  return predicate ? predicate->copy(predicate) : NULL;
}


void cnPredicateDrop(cnPredicate* predicate) {
  if (predicate) {
    if (predicate->dispose) {
      predicate->dispose(predicate);
    }
    free(predicate);
  }
}


cnPredicate* cnPredicateCreateDistanceThreshold_Copy(cnPredicate* predicate) {
  cnPredicateThresholdInfo info = predicate->info;
  cnFunction* function = cnFunctionCopy(info->distanceFunction);
  cnPredicate* copy;
  // Copy even the function because there could be mutable data inside.
  if (!function) return NULL;
  copy = cnPredicateCreateDistanceThreshold(function, info->threshold);
  if (!copy) cnFunctionDrop(function);
  return copy;
}

void cnPredicateCreateDistanceThreshold_Dispose(cnPredicate* predicate) {
  cnPredicateThresholdInfo info = predicate->info;
  cnFunctionDrop(info->distanceFunction);
  free(info);
  predicate->info = NULL;
  predicate->dispose = NULL;
  predicate->evaluate = NULL;
}

cnBool cnPredicateCreateDistanceThreshold_Evaluate(
  cnPredicate* predicate, void* in
) {
  cnPredicateThresholdInfo info = predicate->info;
  cnFloat distance;
  if (
    !info->distanceFunction->evaluate(info->distanceFunction, in, &distance)
  ) {
    // TODO Something better than this abuse.
    return -1;
  }
  // TODO I'd prefer <, but need better a handling of bulks of equal distances
  // TODO in threshold choosing. I've hit the problem before.
  return distance <= info->threshold;
}

cnBool cnPredicateCreateDistanceThreshold_write(
  cnPredicate* predicate, FILE* file, cnString* indent
) {
  cnPredicateThresholdInfo info = predicate->info;
  cnBool result = cnFalse;

  // TODO Check error state?
  fprintf(file, "{\n");
  cnIndent(indent);
  fprintf(file, "%s\"name\": \"DistanceThreshold\",\n", cnStr(indent));

  // Distance function.
  fprintf(file, "%s\"function\": ", cnStr(indent));
  // TODO Check error!
  cnFunctionWrite(info->distanceFunction, file, indent);
  fprintf(file, ",\n");

  // Threshold.
  fprintf(file, "%s\"threshold\": %lg\n", cnStr(indent), info->threshold);

  cnDedent(indent);
  // TODO The real work.
  fprintf(file, "%s}", cnStr(indent));

  // Winned!
  result = cnTrue;

  return result;
}

cnPredicate* cnPredicateCreateDistanceThreshold(
  cnFunction* distanceFunction, cnFloat threshold
) {
  cnPredicate* predicate = malloc(sizeof(cnPredicate));
  cnPredicateThresholdInfo info;
  if (!predicate) {
    return NULL;
  }
  info = malloc(sizeof(struct cnPredicateThresholdInfo));
  if (!info) {
    free(predicate);
    return NULL;
  }
  // Good to go.
  info->distanceFunction = distanceFunction;
  info->threshold = threshold;
  predicate->info = info;
  predicate->copy = cnPredicateCreateDistanceThreshold_Copy;
  predicate->dispose = cnPredicateCreateDistanceThreshold_Dispose;
  predicate->evaluate = cnPredicateCreateDistanceThreshold_Evaluate;
  predicate->write = cnPredicateCreateDistanceThreshold_write;
  return predicate;
}


cnBool cnPredicateWrite(cnPredicate* predicate, FILE* file, cnString* indent) {
  return predicate->write ? predicate->write(predicate, file, indent) : cnFalse;
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
  const cnProperty* property, cnEntity entity, void* storage
) {
  memcpy(
    storage,
    ((char*)entity) + property->offset,
    property->count * property->type->size
  );
}


void cnPropertyFieldPut(
  const cnProperty* property, cnEntity entity, void* value
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
  cnListEachBegin(&schema->types, cnType*, type) {
    cnTypeDrop(*type);
  } cnEnd;
  cnListDispose(&schema->types);
  cnSchemaInit(schema);
}


void cnSchemaInit(cnSchema* schema) {
  schema->floatType = NULL;
  cnListInit(&schema->types, sizeof(cnType*));
}


cnBool cnSchemaInitDefault(cnSchema* schema) {
  cnType *type;
  // Init schema.
  cnSchemaInit(schema);
  // Create float type.
  if (!(type = cnTypeCreate("Float", sizeof(cnFloat)))) {
    cnFailTo(FAIL, "No type.");
  }
  type->schema = schema;
  // Push it on, and keep a nice reference.
  if (!cnListPush(&schema->types, &type)) cnFailTo(FAIL, "Can't push type.");
  schema->floatType = type;
  // We winned!
  return cnTrue;

  FAIL:
  cnTypeDrop(type);
  cnSchemaDispose(schema);
  return cnFalse;
}


cnType* cnTypeCreate(char* name, cnCount size) {
  cnType* type = malloc(sizeof(cnType));
  if (!type) cnFailTo(FAIL, "No type.");
  // Put safety values first.
  type->size = size;
  // Let schema be set later, if wanted.
  type->schema = NULL;
  cnStringInit(&type->name);
  cnListInit(&type->properties, sizeof(cnProperty));
  // Now try things that might fail.
  if (!cnStringPushStr(&type->name, name)) cnFailTo(FAIL, "No type name.");
  return type;

  FAIL:
  cnTypeDrop(type);
  return NULL;
}


void cnTypeDrop(cnType* type) {
  if (!type) return;
  cnStringDispose(&type->name);
  cnListEachBegin(&type->properties, cnProperty, property) {
    cnPropertyDispose(property);
  } cnEnd;
  cnListDispose(&type->properties);
  type->size = 0;
  type->schema = NULL;
  free(type);
}
