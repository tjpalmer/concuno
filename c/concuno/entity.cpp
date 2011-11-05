#include <math.h>
#include <string.h>

#include "entity.h"
#include "io.h"
#include "mat.h"


namespace concuno {


Bag::Bag(): entities(new List<Entity>), label(false) {}


Bag::Bag(List<Entity>* $entities): entities($entities), label(false) {}


Bag::~Bag() {
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
    new(participantOptions) List<Entity>;
  }

  // Expand the one for the right depth.
  participantOptions = &this->participantOptions[depth];
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
    bag->~Bag();
  } cnEnd;
  // Lists in the bags.
  if (entityLists) {
    cnListEachBegin(entityLists, List<Entity>*, list) {
      cnListDestroy(*list);
    } cnEnd;
  }
}


ComposedEntityFunction::ComposedEntityFunction(
  EntityFunction& $base, const char* name, Count inCount, Count outCount
):
  EntityFunction(name, inCount, outCount), base($base)
{
  outTopology = base.outTopology;
  // TODO Verify non-nulls?
  //printf("outType: %s\n", base.outType->name.c_str());
  if (base.outType != base.outType->schema->floatType) {
    // TODO Supply a difference function for generic handling?
    // If this dispose follows the nulled dispose pointer, we're okay.
    // Oh, and init'd name is also important.
    throw Error("Only works for floats so far.");
  }
  outType = base.outType;
  this->name.append(base.name);
}


void DifferenceEntityFunction::get(
  EntityFunction& base, Entity* ins, void* outs
) {
  // TODO Remove float assumption here.
  // Vectors are assumed small, so use stack memory.
  Float* x = cnStackAllocOf(Float, base.outCount);
  Float* result = reinterpret_cast<Float*>(outs);
  if (!x) {
    throw Error("No working space.");
  }
  base.get(ins, result);
  base.get(ins + 1, x);
  for (Index i = 0; i < base.outCount; i++) {
    result[i] -= x[i];
  }
  cnStackFree(x);
}


DifferenceEntityFunction::DifferenceEntityFunction(EntityFunction& base):
  ComposedEntityFunction(base, "Difference", 2, base.outCount)
{}


void DifferenceEntityFunction::get(Entity* ins, void* outs) {
  // Our assumption that base.outCount == this->outCount allows the use
  // of base here. And the use of base here allows this difference function to
  // be used directly by distance as well.
  get(base, ins, outs);
}


DistanceEntityFunction::DistanceEntityFunction(EntityFunction& base):
  ComposedEntityFunction(base, "Distance", 2, 1)
{}


void DistanceEntityFunction::get(Entity* ins, void* outs) {
  // TODO Remove float assumption here.
  // Vectors are assumed small, so use stack memory.
  Float* diff = cnStackAllocOf(Float, outCount);
  Float* result = reinterpret_cast<Float*>(outs);
  if (!diff) {
    throw Error("No working space.");
  }
  // Get the difference.
  DifferenceEntityFunction::get(base, ins, diff);
  // Get the norm of the difference.
  // Using sqrt leaves results clearer and has a very small cost.
  *result = cnNorm(outCount, diff);
  cnStackFree(diff);
}


EntityFunction::EntityFunction(
  const char* $name, Count $inCount, Count $outCount
):
  inCount($inCount), name($name),
  outCount($outCount), outTopology(Topology::Euclidean), outType(NULL)
{}


EntityFunction::~EntityFunction() {}


void cnEntityFunctionDrop(EntityFunction* function) {
  delete function;
}


PropertyEntityFunction::PropertyEntityFunction(Property& $property):
  EntityFunction($property.name.c_str(), 1, $property.count),
  property($property)
{
  outTopology = $property.topology;
  outType = $property.type;
}


void PropertyEntityFunction::get(Entity* ins, void* outs) {
  if (!*ins) {
    // Provide NaNs for floats when no input given.
    // TODO See if throwing works.
    // TODO Anything for other types? Error result?
    if (outType == outType->schema->floatType) {
      Float nan = cnNaN();
      Index o;
      for (o = 0; o < outCount; o++) {
        ((Float*)outs)[o] = nan;
      }
    }
  } else {
    property.get(*ins, outs);
  }
}


ReframeEntityFunction::ReframeEntityFunction(EntityFunction& base):
  ComposedEntityFunction(base, "Reframe", 3, base.outCount)
{}


void ReframeEntityFunction::get(Entity* ins, void* outs) {
  // Vectors are assumed small, so use stack memory.
  // Our assumption that base->outCount == function->outCount allows the use
  // of base here.
  Float* origin = cnStackAllocOf(Float, 2 * outCount);
  Float* target = origin + outCount;
  Float* result = reinterpret_cast<Float*>(outs);
  if (!origin) {
    throw Error("No working space.");
  }

  // First param specifies new frame origin. Second (target) is the new
  // (1,0,0,...) vector, or in other words, one unit down the x axis.
  base.get(ins, origin);
  base.get(ins + 1, target);
  base.get(ins + 2, result);

  // Now transform.
  reframe(outCount, origin, target, result);

  // Free origin, and target goes bundled along.
  cnStackFree(origin);
}


ValidityEntityFunction::ValidityEntityFunction(Schema* schema, Count arity):
  EntityFunction("Valid", arity, 1)
{
  outType = schema->floatType; // TODO Integer or some such?
}


void ValidityEntityFunction::get(Entity* ins, void* outs) {
  Float* out = reinterpret_cast<Float*>(outs);
  for (Index i = 0; i < inCount; i++) {
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


FieldProperty::FieldProperty(
  Type* containerType, Type* type, const char* name, Count $offset, Count count
): Property(containerType, type, name, count), offset($offset) {}


void FieldProperty::get(Entity entity, void* storage) {
  memcpy(storage, ((char*)entity) + offset, count * type->size);
}


void FieldProperty::put(Entity entity, void* value) {
  memcpy(((char*)entity) + offset, value, count * type->size);
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
EntityFunction* pushOrDeleteFunction(
  List<EntityFunction*>* functions, EntityFunction* function
) {
  // So far, so good.
  if (!cnListPush(functions, &function)) {
    // Nope, we failed.
    delete function;
    throw Error("Failed to push function.");
  }
  return function;
}


EntityFunction* cnPushDifferenceFunction(
  List<EntityFunction*>* functions, EntityFunction* base
) {
  return pushOrDeleteFunction(functions, new DifferenceEntityFunction(*base));
}


EntityFunction* cnPushDistanceFunction(
  List<EntityFunction*>* functions, EntityFunction* base
) {
  return pushOrDeleteFunction(functions, new DistanceEntityFunction(*base));
}


EntityFunction* cnPushPropertyFunction(
  List<EntityFunction*>* functions, Property* property
) {
  return pushOrDeleteFunction(functions, new PropertyEntityFunction(*property));
}


EntityFunction* cnPushValidFunction(
  List<EntityFunction*>* functions, Schema* schema, Count arity
) {
  return
    pushOrDeleteFunction(functions, new ValidityEntityFunction(schema, arity));
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


Property::Property(
  Type* $containerType, Type* $type, const char* $name, Count $count
):
  containerType($containerType), type($type), name($name),
  topology(Topology::Euclidean), count($count)
{}


Property::~Property() {}


Schema::Schema(): floatType(NULL) {}


Schema::~Schema() {
  cnListEachBegin(&types, Type*, type) {
    delete *type;
  } cnEnd;
}


void cnSchemaInitDefault(Schema* schema) {
  // Create float type.
  Type* type = new Type("Float", sizeof(Float));
  type->schema = schema;
  // Push it on, and keep a nice reference.
  schema->types.pushOrDelete(type);
  schema->floatType = type;
}


Type::Type(const char* $name, Count $size):
  name($name),
  // Let schema be set later, if wanted.
  schema(NULL),
  size($size)
{}


void cnTypeDrop(Type* type) {
  if (!type) return;
  free(type);
}


}
