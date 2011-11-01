#include <math.h>
#include <string.h>

#include "entity.h"
#include "io.h"
#include "mat.h"


namespace concuno {


Bag::Bag() {
  init();
}


Bag::Bag(List<Entity>* $entities) {
  init($entities);
}


Bag::~Bag() {
  dispose();
}


void Bag::dispose() {
  // If managed elsewhere, the list might be nulled, so check it.
  if (entities) {
    delete entities;
  }

  // Participants are always owned by the bag, however.
  // TODO Really? By allowing outside control, can it be made more efficient?
  cnListEachBegin(&participantOptions, List<Entity>, list) {
    // These would need done manually because allocation is done in unofficial
    // form. Nothing knows they're being destroyed.
    list->dispose();
  } cnEnd;

  // Other stuff.
  label = false;
  entities = NULL;
}


void Bag::init(List<Entity>* $entities) {
  // Safety first.
  label = false;
  entities = $entities ? $entities : new List<Entity>;
}


void Bag::pushParticipant(Index depth, Entity participant) {
  List<Entity>* participantOptions;

  // Grow more lists if needed.
  while (depth >= this->participantOptions.count) {
    if (!(participantOptions = reinterpret_cast<List<Entity>*>(
      cnListExpand(&this->participantOptions)
    ))) {
      throw Error("Failed to grow participant options.");
    }
    participantOptions->init();
  }

  // Expand the one for the right depth.
  participantOptions = reinterpret_cast<List<Entity>*>(
    cnListGet(&this->participantOptions, depth)
  );
  if (!cnListPush(participantOptions, &participant)) {
    throw Error("Failed to push participant.");
  }
}


void cnBagListDispose(
  List<Bag>* bags, List<List<Entity>*>* entityLists
) {
  // Bags.
  cnListEachBegin(bags, Bag, bag) {
    // Dispose the bag, but first hide entities if we manage them separately.
    if (entityLists) bag->entities = NULL;
    bag->dispose();
  } cnEnd;
  // Lists in the bags.
  if (entityLists) {
    cnListEachBegin(entityLists, List<Entity>*, list) {
      cnListDestroy(*list);
    } cnEnd;
  }
}


void cnEntityFunctionCreateDifference_get(
  EntityFunction* function, Entity* ins, void* outs
) {
  // TODO Remove float assumption here.
  Index i;
  EntityFunction* base = reinterpret_cast<EntityFunction*>(function->data);
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

EntityFunction* cnEntityFunctionCreateDifference(EntityFunction* base) {
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) return NULL;
  function->data = base;
  function->dispose = NULL;
  function->inCount = 2;
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
  function->name.append("Difference").append(base->name);
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateDistance_get(
  EntityFunction* function, Entity* ins, void* outs
) {
  // TODO Remove float assumption here.
  Index i;
  EntityFunction* base = reinterpret_cast<EntityFunction*>(function->data);
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

EntityFunction* cnEntityFunctionCreateDistance(EntityFunction* base) {
  // TODO Combine setup with difference? Differences indicated below.
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) return NULL;
  function->data = base;
  function->dispose = NULL;
  function->inCount = 2;
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
  function->name.append("Distance").append(base->name); // Also different!
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateProperty_get(
  EntityFunction* function, Entity* ins, void* outs
) {
  Property* property = reinterpret_cast<Property*>(function->data);
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

EntityFunction* cnEntityFunctionCreateProperty(Property* property) {
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) return NULL;
  function->data = property;
  function->dispose = NULL;
  function->inCount = 1;
  function->outCount = property->count;
  function->outTopology = property->topology;
  function->outType = property->type;
  function->get = cnEntityFunctionCreateProperty_get;
  function->name.append(cnStr((String*)&property->name));
  return function;
}


void cnEntityFunctionCreateReframe_get(
  EntityFunction* function, Entity* ins, void* outs
) {
  // Some work here based on the stable computation technique for Givens
  // rotations at Wikipedia: http://en.wikipedia.org/wiki/Givens_rotation
  Index i;
  EntityFunction* base = reinterpret_cast<EntityFunction*>(function->data);
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
  reframe(base->outCount, origin, target, result);

  // Free origin, and target goes bundled along.
  cnStackFree(origin);
}

EntityFunction* cnEntityFunctionCreateReframe(EntityFunction* base) {
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) return NULL;
  function->data = base;
  function->dispose = NULL;
  function->inCount = 3;
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
  function->name.append("Reframe").append(base->name); // Also different!
  return function;

  FAIL:
  cnEntityFunctionDrop(function);
  return NULL;
}


void cnEntityFunctionCreateValid_get(
  EntityFunction* function, Entity* ins, void* outs
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

EntityFunction* cnEntityFunctionCreateValid(Schema* schema, Count arity) {
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) return NULL;
  function->data = NULL;
  function->dispose = NULL;
  function->inCount = arity;
  function->outCount = 1;
  function->outTopology = TopologyEuclidean; // TODO Discrete or ordinal?
  function->outType = schema->floatType; // TODO Integer or some such?
  function->get = cnEntityFunctionCreateValid_get;
  // The one thing that can fail directly here.
  // TODO Customize name by arity?
  function->name.append("Valid");
  return function;
}


EntityFunction::EntityFunction(
  const char* $name, Count $inCount, Count $outCount
):
  data(NULL), inCount($inCount), name($name), outCount($outCount),
  outTopology(TopologyEuclidean), outType(NULL), dispose(NULL), get(NULL)
{}


EntityFunction::~EntityFunction() {
  if (dispose) {
    dispose(this);
  }
}


void cnEntityFunctionDrop(EntityFunction* function) {
  if (!function) return;
  function->~EntityFunction();
  free(function);
}


EntityFunction* cnEntityFunctionCreate(
  const char* name, Count inCount, Count outCount
) {
  // TODO Replace with new!
  EntityFunction* function = cnAlloc(EntityFunction, 1);
  if (!function) throw Error("No function.");
  new(function) EntityFunction(name, inCount, outCount);
  return function;
}


Function* cnFunctionCopy(Function* function) {
  return function ? function->copy(function) : NULL;
}


void cnFunctionDrop(Function* function) {
  if (function) {
    if (function->dispose) {
      function->dispose(function);
    }
    free(function);
  }
}


bool cnFunctionWrite(Function* function, FILE* file, String* indent) {
  return function->write ? function->write(function, file, indent) : false;
}


/**
 * A helper for various composite entity functions.
 */
EntityFunction* cnPushCompositeFunction(
  List<EntityFunction*>* functions,
  EntityFunction* (*wrapper)(EntityFunction* base),
  EntityFunction* base
) {
  EntityFunction* function;
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


EntityFunction* cnPushDifferenceFunction(
  List<EntityFunction*>* functions, EntityFunction* base
) {
  return
    cnPushCompositeFunction(functions, cnEntityFunctionCreateDifference, base);
}


EntityFunction* cnPushDistanceFunction(
  List<EntityFunction*>* functions, EntityFunction* base
) {
  return
    cnPushCompositeFunction(functions, cnEntityFunctionCreateDistance, base);
}


EntityFunction* cnPushPropertyFunction(
  List<EntityFunction*>* functions, Property* property
) {
  EntityFunction* function;
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


EntityFunction* cnPushValidFunction(
  List<EntityFunction*>* functions, Schema* schema, Count arity
) {
  EntityFunction* function;
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


Predicate* cnPredicateCopy(Predicate* predicate) {
  return predicate ? predicate->copy(predicate) : NULL;
}


void cnPredicateDrop(Predicate* predicate) {
  if (predicate) {
    if (predicate->dispose) {
      predicate->dispose(predicate);
    }
    free(predicate);
  }
}


Predicate* cnPredicateCreateDistanceThreshold_Copy(Predicate* predicate) {
  PredicateThresholdInfo* info =
    reinterpret_cast<PredicateThresholdInfo*>(predicate->info);
  Function* function = cnFunctionCopy(info->distanceFunction);
  Predicate* copy;
  // Copy even the function because there could be mutable data inside.
  if (!function) return NULL;
  copy = cnPredicateCreateDistanceThreshold(function, info->threshold);
  if (!copy) cnFunctionDrop(function);
  return copy;
}

void cnPredicateCreateDistanceThreshold_Dispose(Predicate* predicate) {
  PredicateThresholdInfo* info =
    reinterpret_cast<PredicateThresholdInfo*>(predicate->info);
  cnFunctionDrop(info->distanceFunction);
  free(info);
  predicate->info = NULL;
  predicate->dispose = NULL;
  predicate->evaluate = NULL;
}

bool cnPredicateCreateDistanceThreshold_Evaluate(
  Predicate* predicate, void* in
) {
  PredicateThresholdInfo* info =
    reinterpret_cast<PredicateThresholdInfo*>(predicate->info);
  Float distance;
  if (
    !info->distanceFunction->evaluate(info->distanceFunction, in, &distance)
  ) {
    throw Error("Failed to evaluate distance function.");
  }
  // TODO I'd prefer <, but need better a handling of bulks of equal distances
  // TODO in threshold choosing. I've hit the problem before.
  return distance <= info->threshold;
}

bool cnPredicateCreateDistanceThreshold_write(
  Predicate* predicate, FILE* file, String* indent
) {
  PredicateThresholdInfo* info =
    reinterpret_cast<PredicateThresholdInfo*>(predicate->info);
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

Predicate* cnPredicateCreateDistanceThreshold(
  Function* distanceFunction, Float threshold
) {
  Predicate* predicate = cnAlloc(Predicate, 1);
  PredicateThresholdInfo* info;
  if (!predicate) {
    return NULL;
  }
  info = cnAlloc(PredicateThresholdInfo, 1);
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


bool cnPredicateWrite(Predicate* predicate, FILE* file, String* indent) {
  return predicate->write ? predicate->write(predicate, file, indent) : false;
}


void cnPropertyDispose(Property* property) {
  // Dispose of extra data, as needed.
  if (property->dispose) {
    property->dispose(property);
    property->dispose = NULL;
  }
  // Also 0 out the offset. As a union, this might be unneeded and/or
  // insufficient, but here goes.
  property->offset = 0;
  property->data = NULL;
  // Clear out the simple things.
  property->count = 0;
  property->get = NULL;
  property->put = NULL;
  property->topology = TopologyEuclidean;
  property->type = NULL;
}


void cnPropertyFieldGet(Property* property, Entity entity, void* storage) {
  memcpy(
    storage,
    ((char*)entity) + property->offset,
    property->count * property->type->size
  );
}


void cnPropertyFieldPut(Property* property, Entity entity, void* value) {
  memcpy(
    ((char*)entity) + property->offset,
    value,
    property->count * property->type->size
  );
}


bool cnPropertyInitField(
  Property* property, Type* containerType, Type* type, const char* name,
  Count offset, Count count
) {
  // Safety items first.
  property->name.init();
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
  property->topology = TopologyEuclidean;
  property->type = type;
  return true;
}


void cnSchemaDispose(Schema* schema) {
  cnListEachBegin(&schema->types, Type*, type) {
    cnTypeDrop(*type);
  } cnEnd;
  cnSchemaInit(schema);
}


void cnSchemaInit(Schema* schema) {
  schema->floatType = NULL;
  schema->types.init(sizeof(Type*));
}


bool cnSchemaInitDefault(Schema* schema) {
  Type *type;
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


Type::Type(const char* $name, Count $size):
  name($name),
  // Let schema be set later, if wanted.
  schema(NULL),
  size($size)
{}


Type::~Type() {
  cnListEachBegin(&properties, Property, property) {
    cnPropertyDispose(property);
  } cnEnd;
}


Type* cnTypeCreate(const char* name, Count size) {
  Type* type = cnAlloc(Type, 1);
  if (!type) throw Error("No type.");
  new(type) Type(name, size);
  return type;
}


void cnTypeDrop(Type* type) {
  if (!type) return;
  type->~Type();
  free(type);
}


}
