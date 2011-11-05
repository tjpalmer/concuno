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


DistanceThresholdPredicate::DistanceThresholdPredicate(
  Function* $distanceFunction, Float $threshold
): distanceFunction($distanceFunction), threshold($threshold) {}


DistanceThresholdPredicate::~DistanceThresholdPredicate() {
  delete distanceFunction;
}


Predicate* DistanceThresholdPredicate::copy() {
  // Copy even the function because there could be mutable data inside.
  Function* function = distanceFunction->copy();
  try {
    return new DistanceThresholdPredicate(function, threshold);
  } catch (const std::exception& e) {
    delete function;
    throw;
  }
}


bool DistanceThresholdPredicate::evaluate(void* in) {
  Float distance;
  distanceFunction->evaluate(in, &distance);
  // TODO I'd prefer <, but need better a handling of bulks of equal distances
  // TODO in threshold choosing. I've hit the problem before.
  return distance <= threshold;
}


void DistanceThresholdPredicate::write(FILE* file, String* indent) {
  // TODO Check error state?
  fprintf(file, "{\n");
  cnIndent(indent);
  fprintf(file, "%s\"name\": \"DistanceThreshold\",\n", cnStr(indent));

  // Distance function.
  fprintf(file, "%s\"function\": ", cnStr(indent));
  // TODO Check error!
  distanceFunction->write(file, indent);
  fprintf(file, ",\n");

  // Threshold.
  fprintf(file, "%s\"threshold\": %lg\n", cnStr(indent), threshold);

  cnDedent(indent);
  // TODO The real work.
  fprintf(file, "%s}", cnStr(indent));
}


EntityFunction::EntityFunction(
  const char* $name, Count $inCount, Count $outCount
):
  inCount($inCount), name($name),
  outCount($outCount), outTopology(Topology::Euclidean), outType(NULL)
{}


EntityFunction::~EntityFunction() {}


void EntityFunction::pushOrDelete(std::vector<EntityFunction*>& functions) {
  concuno::pushOrDelete(functions, this);
}


FieldProperty::FieldProperty(
  Type* containerType, Type* type, const char* name, Count count
): Property(containerType, type, name, count) {}


void FieldProperty::get(Entity entity, void* storage) {
  memcpy(storage, reinterpret_cast<char*>(field(entity)), count * type->size);
}


void FieldProperty::put(Entity entity, void* value) {
  memcpy(reinterpret_cast<char*>(field(entity)), value, count * type->size);
}


Function::~Function() {}


OffsetProperty::OffsetProperty(
  Type* containerType, Type* type, const char* name, Count $offset, Count count
): Property(containerType, type, name, count), offset($offset) {}


void OffsetProperty::get(Entity entity, void* storage) {
  memcpy(storage, ((char*)entity) + offset, count * type->size);
}


void OffsetProperty::put(Entity entity, void* value) {
  memcpy(((char*)entity) + offset, value, count * type->size);
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


ValidityEntityFunction::ValidityEntityFunction(Schema& schema, Count arity):
  EntityFunction("Valid", arity, 1)
{
  outType = schema.floatType; // TODO Integer or some such?
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


Predicate::~Predicate() {}


Property::Property(
  Type* $containerType, Type* $type, const char* $name, Count $count
):
  containerType($containerType), type($type), name($name),
  topology(Topology::Euclidean), count($count)
{}


Property::~Property() {}


Schema::Schema(): floatType(new Type(*this, "Float", sizeof(Float))) {
  // Also push on the floatType for proper management.
  pushOrDelete(*types, floatType);
}


Type::Type(Schema& $schema, const char* $name, Count $size):
  name($name),
  // Let schema be set later, if wanted.
  schema(&$schema),
  size($size)
{}


}
