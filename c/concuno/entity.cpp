#include <math.h>
#include <string.h>

#include "entity.h"
#include "io.h"
#include "mat.h"


namespace concuno {


Bag::Bag() {
  init();
}


Bag::Bag(cnList(cnEntity)* $entities) {
  init($entities);
}


Bag::~Bag() {
  dispose();
}


void Bag::dispose() {
  // If managed elsewhere, the list might be nulled, so check it.
  if (entities) {
    cnListDispose(entities);
    free(entities);
  }

  // Participants are always owned by the bag, however.
  // TODO Really? By allowing outside control, can it be made more efficient?
  cnListEachBegin(&participantOptions, cnList(cnEntity), list) {
    cnListDispose(list);
  } cnEnd;
  cnListDispose(&participantOptions);

  // Other stuff.
  label = false;
  entities = NULL;
}


void Bag::init(cnList(cnEntity)* $entities) {
  // Safety first.
  label = false;
  entities = $entities ? $entities : cnAlloc(cnList(cnEntity), 1);
  cnListInit(&participantOptions, sizeof(cnList(cnEntity)));

  // Check on and finish up entities.
  if (!entities) throw "No bag entities.";
  if (!$entities) cnListInit(entities, sizeof(cnEntity));
}


void Bag::pushParticipant(Index depth, cnEntity participant) {
  cnList(cnEntity)* participantOptions;

  // Grow more lists if needed.
  while (depth >= this->participantOptions.count) {
    if (!(participantOptions = reinterpret_cast<cnList(cnEntity)*>(
      cnListExpand(&this->participantOptions)
    ))) {
      throw "Failed to grow participant options.";
    }
    cnListInit(participantOptions, sizeof(cnEntity));
  }

  // Expand the one for the right depth.
  participantOptions = reinterpret_cast<cnList(cnEntity)*>(
    cnListGet(&this->participantOptions, depth)
  );
  if (!cnListPush(participantOptions, &participant)) {
    throw "Failed to push participant.";
  }
}


void cnBagListDispose(
  cnList(Bag)* bags, cnList(cnList(cnEntity)*)* entityLists
) {
  // Bags.
  cnListEachBegin(bags, Bag, bag) {
    // Dispose the bag, but first hide entities if we manage them separately.
    if (entityLists) bag->entities = NULL;
    bag->dispose();
  } cnEnd;
  cnListDispose(bags);
  // Lists in the bags.
  if (entityLists) {
    cnListEachBegin(entityLists, cnList(cnEntity)*, list) {
      cnListDestroy(*list);
    } cnEnd;
    cnListDispose(entityLists);
  }
}


void cnEntityFunctionCreateDifference_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  // TODO Remove float assumption here.
  Index i;
  cnEntityFunction* base = reinterpret_cast<cnEntityFunction*>(function->data);
  // Vectors are assumed small, so use stack memory.
  // Our assumption that base->outCount == function->outCount allows the use
  // of base here. And the use of base here allows this difference function to
  // be used directly by distance as well.
  Float* x = cnStackAllocOf(Float, base->outCount);
  Float* result = reinterpret_cast<Float*>(outs);
  if (!x) {
    // TODO Some way to report errors. Just NaN out for now.
    Float nan = cnNaN();
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
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
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
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnErrTo(FAIL, "Only works for floats so far.");
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionCreateDifference_get;
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Difference")) {
    cnFailTo(FAIL);
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnFailTo(FAIL);
  }
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateDistance_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  // TODO Remove float assumption here.
  Index i;
  cnEntityFunction* base = reinterpret_cast<cnEntityFunction*>(function->data);
  // Vectors are assumed small, so use stack memory.
  Float* diff = cnStackAllocOf(Float, base->outCount);
  Float* result = reinterpret_cast<Float*>(outs);
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
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
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
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnErrTo(FAIL, "Only works for floats so far.");
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionCreateDistance_get; // Also different!
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Distance")) { // Also different!
    cnFailTo(FAIL);
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnFailTo(FAIL);
  }
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateProperty_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  cnProperty* property = reinterpret_cast<cnProperty*>(function->data);
  if (!*ins) {
    // Provide NaNs for floats when no input given.
    // TODO Anything for other types? Error result?
    if (function->outType == function->outType->schema->floatType) {
      Float nan = cnNaN();
      Index o;
      for (o = 0; o < function->outCount; o++) {
        ((Float*)outs)[o] = nan;
      }
    }
  } else {
    property->get(property, *ins, outs);
  }
}

cnEntityFunction* cnEntityFunctionCreateProperty(cnProperty* property) {
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
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


void cnEntityFunctionCreateReframe_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  // Some work here based on the stable computation technique for Givens
  // rotations at Wikipedia: http://en.wikipedia.org/wiki/Givens_rotation
  Index i;
  cnEntityFunction* base = reinterpret_cast<cnEntityFunction*>(function->data);
  // Vectors are assumed small, so use stack memory.
  // Our assumption that base->outCount == function->outCount allows the use
  // of base here.
  Float* origin = cnStackAllocOf(Float, 2 * base->outCount);
  Float* target = origin + base->outCount;
  Float* result = reinterpret_cast<Float*>(outs);
  if (!(origin && target)) {
    // TODO Some way to report errors. Just NaN out for now.
    Float nan = cnNaN();
    if (origin) cnStackFree(origin);
    for (i = 0; i < base->outCount; i++) {
      result[i] = nan;
    }
    return;
  }

  // First param specifies new frame origin. Second (target) is the new
  // (1,0,0,...) vector, or in other words, one unit down the x axis.
  base->get(base, ins, origin);
  base->get(base, ins + 1, target);
  base->get(base, ins + 2, result);

  // Now transform.
  cnReframe(base->outCount, origin, target, result);

  // Free origin, and target goes bundled along.
  cnStackFree(origin);
}

cnEntityFunction* cnEntityFunctionCreateReframe(cnEntityFunction* base) {
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
  if (!function) return NULL;
  function->data = base;
  function->dispose = NULL;
  function->inCount = 3;
  cnStringInit(&function->name);
  function->outCount = base->outCount; // TODO For all topologies?
  function->outTopology = base->outTopology;
  // TODO Verify non-nulls?
  //printf("outType: %s\n", cnStr(&base->outType->name));
  if (base->outType != base->outType->schema->floatType) {
    // TODO Supply a difference function for generic handling?
    // TODO Is a difference function enough for reframe or do we need extra
    // TODO knowledge?
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    cnErrTo(FAIL, "Only works for floats so far.");
  }
  function->outType = base->outType;
  function->get = cnEntityFunctionCreateReframe_get;
  // Deal with this last, to make sure everything else is sane first.
  if (!cnStringPushStr(&function->name, "Reframe")) {
    cnFailTo(FAIL);
  }
  if (!cnStringPushStr(&function->name, cnStr((cnString*)&base->name))) {
    cnFailTo(FAIL);
  }
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateValid_get(
  cnEntityFunction* function, cnEntity* ins, void* outs
) {
  Index i;
  Float* out = reinterpret_cast<Float*>(outs);
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

cnEntityFunction* cnEntityFunctionCreateValid(cnSchema* schema, Count arity) {
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
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
  const char* name, Count inCount, Count outCount
) {
  cnEntityFunction* function = cnAlloc(cnEntityFunction, 1);
  if (!function) cnErrTo(DONE, "No function.");
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
    cnErrTo(DONE, "No function name.");
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


bool cnFunctionWrite(cnFunction* function, FILE* file, cnString* indent) {
  return function->write ? function->write(function, file, indent) : false;
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
  cnList(cnEntityFunction*)* functions, cnSchema* schema, Count arity
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
  cnPredicateThresholdInfo info =
    reinterpret_cast<cnPredicateThresholdInfo>(predicate->info);
  cnFunction* function = cnFunctionCopy(info->distanceFunction);
  cnPredicate* copy;
  // Copy even the function because there could be mutable data inside.
  if (!function) return NULL;
  copy = cnPredicateCreateDistanceThreshold(function, info->threshold);
  if (!copy) cnFunctionDrop(function);
  return copy;
}

void cnPredicateCreateDistanceThreshold_Dispose(cnPredicate* predicate) {
  cnPredicateThresholdInfo info =
    reinterpret_cast<cnPredicateThresholdInfo>(predicate->info);
  cnFunctionDrop(info->distanceFunction);
  free(info);
  predicate->info = NULL;
  predicate->dispose = NULL;
  predicate->evaluate = NULL;
}

bool cnPredicateCreateDistanceThreshold_Evaluate(
  cnPredicate* predicate, void* in
) {
  cnPredicateThresholdInfo info =
    reinterpret_cast<cnPredicateThresholdInfo>(predicate->info);
  Float distance;
  if (
    !info->distanceFunction->evaluate(info->distanceFunction, in, &distance)
  ) {
    throw "Failed to evaluate distance function.";
  }
  // TODO I'd prefer <, but need better a handling of bulks of equal distances
  // TODO in threshold choosing. I've hit the problem before.
  return distance <= info->threshold;
}

bool cnPredicateCreateDistanceThreshold_write(
  cnPredicate* predicate, FILE* file, cnString* indent
) {
  cnPredicateThresholdInfo info =
    reinterpret_cast<cnPredicateThresholdInfo>(predicate->info);
  bool result = false;

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
  result = true;

  return result;
}

cnPredicate* cnPredicateCreateDistanceThreshold(
  cnFunction* distanceFunction, Float threshold
) {
  cnPredicate* predicate = cnAlloc(cnPredicate, 1);
  cnPredicateThresholdInfo info;
  if (!predicate) {
    return NULL;
  }
  info = cnAlloc(struct $cnPredicateThresholdInfo, 1);
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


bool cnPredicateWrite(cnPredicate* predicate, FILE* file, cnString* indent) {
  return predicate->write ? predicate->write(predicate, file, indent) : false;
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


void cnPropertyFieldGet(cnProperty* property, cnEntity entity, void* storage) {
  memcpy(
    storage,
    ((char*)entity) + property->offset,
    property->count * property->type->size
  );
}


void cnPropertyFieldPut(cnProperty* property, cnEntity entity, void* value) {
  memcpy(
    ((char*)entity) + property->offset,
    value,
    property->count * property->type->size
  );
}


bool cnPropertyInitField(
  cnProperty* property, cnType* containerType, cnType* type, const char* name,
  Count offset, Count count
) {
  // Safety items first.
  cnStringInit(&property->name);
  property->dispose = NULL;
  // Now other things.
  if (!cnStringPushStr(&property->name, name)) {
    cnPropertyDispose(property);
    return false;
  }
  property->containerType = containerType;
  property->count = count;
  property->get = cnPropertyFieldGet;
  property->offset = offset;
  property->put = cnPropertyFieldPut;
  property->topology = cnTopologyEuclidean;
  property->type = type;
  return true;
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


bool cnSchemaInitDefault(cnSchema* schema) {
  cnType *type;
  // Init schema.
  cnSchemaInit(schema);
  // Create float type.
  if (!(type = cnTypeCreate("Float", sizeof(Float)))) {
    cnErrTo(FAIL, "No type.");
  }
  type->schema = schema;
  // Push it on, and keep a nice reference.
  if (!cnListPush(&schema->types, &type)) cnErrTo(FAIL, "Can't push type.");
  schema->floatType = type;
  // We winned!
  return true;

  FAIL:
  cnTypeDrop(type);
  cnSchemaDispose(schema);
  return false;
}


cnType* cnTypeCreate(const char* name, Count size) {
  cnType* type = cnAlloc(cnType, 1);
  if (!type) cnErrTo(FAIL, "No type.");
  // Put safety values first.
  type->size = size;
  // Let schema be set later, if wanted.
  type->schema = NULL;
  cnStringInit(&type->name);
  cnListInit(&type->properties, sizeof(cnProperty));
  // Now try things that might fail.
  if (!cnStringPushStr(&type->name, name)) cnErrTo(FAIL, "No type name.");
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


}
