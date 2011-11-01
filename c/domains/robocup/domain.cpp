#include <stddef.h>
#include <string.h>
#include "domain.h"

using namespace concuno;


namespace ccndomain {
namespace rcss {


enum cnrFieldType {

  /**
   * These are of type 'int'.
   */
  cnrFieldTypeEnum,

  /**
   * These are of type 'Int' (usually 'long').
   */
  cnrFieldTypeInt,

  /**
   * Type Float.
   */
  cnrFieldTypeFloat,

};


struct FieldInfo {

  cnrFieldType fieldType;

  Item::Type itemType;

  Count offset;

};


Ball::Ball(): Item(Item::TypeBall) {}


Game::~Game() {
  // Dispose states.
  cnListEachBegin(&states, State, state) {
    state->~State();
  } cnEnd;
}


Item::Item(Item::Type $type): type($type) {
  // TODO Any way to construct this array to zeros?
  location[0] = 0.0;
  location[1] = 0.0;
}


Player::Player():
  Item(Item::TypePlayer),
  // No kick to start with.
  kickAngle(cnNaN()), kickPower(cnNaN()),
  // Actual players start from 1, so 0 is a good invalid value.
  index(0),
  // On the other hand, I expect 0 and 1 both for teams, so use -1 for bogus.
  team(-1)
{}


void cnrPropertyFieldGet(Property* property, Entity entity, void* storage) {
  FieldInfo* info = (FieldInfo*)property->data;
  Item* item = reinterpret_cast<Item*>(entity);
  if (info->itemType != Item::TypeAny && item->type != info->itemType) {
    // Types don't match. Give NaNs.
    // TODO If we were more explicit, could we be more efficient in learning?
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + property->count;
    for (; out < outEnd; out++) {
      *((Float*)storage) = cnNaN();
    }
  } else if (info->fieldType == cnrFieldTypeEnum) {
    // Out must be float.
    unsigned int* in = (unsigned int*)(((char*)entity) + info->offset);
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else if (info->fieldType == cnrFieldTypeInt) {
    // Out must be float.
    Int* in = (Int*)(((char*)entity) + info->offset);
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else {
    // Just copy.
    memcpy(
      storage,
      ((char*)entity) + info->offset,
      property->count * property->type->size
    );
  }
}


void cnrPropertyFieldPut(Property* property, Entity entity, void* value) {
  FieldInfo* info = (FieldInfo*)property->data;
  Item* item = reinterpret_cast<Item*>(entity);
  if (info->itemType != Item::TypeAny && item->type != info->itemType) {
    // Types don't match. Nothing to do here.
    // TODO Any way to indicate error?
  } else if (info->fieldType == cnrFieldTypeEnum) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    unsigned int* out = (unsigned int*)(((char*)entity) + info->offset);
    unsigned int* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = (unsigned int)*in;
    }
  } else if (info->fieldType == cnrFieldTypeInt) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    Int* out = (Int*)(((char*)entity) + info->offset);
    Int* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = (Int)*in;
    }
  } else {
    // Just copy.
    memcpy(
      ((char*)entity) + info->offset,
      value,
      property->count * property->type->size
    );
  }
}


void cnrPropertyFieldInfoDispose(Property* property) {
  // TODO Should this be in entity.c? It seems somewhat generally useful.
  free(property->data);
}


bool cnrPropertyInitTypedField(
  Property* property, Type* containerType, Item::Type itemType,
  Type* type, cnrFieldType fieldType,
  const char* name, Count offset, Count count
) {
  FieldInfo* info;
  // Safety items first.
  property->name.init();
  property->dispose = NULL;
  // Failing things.
  if (!cnStringPushStr(&property->name, name)) cnFailTo(FAIL);
  if (!(
    info = reinterpret_cast<FieldInfo*>(malloc(sizeof(struct FieldInfo)))
  )) {
    cnErrTo(FAIL, "Failed to allocate field info.");
  }
  info->fieldType = fieldType;
  info->itemType = itemType;
  info->offset = offset;
  // Easy things.
  property->containerType = containerType;
  property->count = count;
  property->data = info;
  property->dispose = cnrPropertyFieldInfoDispose;
  property->get = cnrPropertyFieldGet;
  property->put = cnrPropertyFieldPut;
  property->topology = TopologyEuclidean;
  property->type = type;
  return true;

  FAIL:
  cnPropertyDispose(property);
  return false;
}


bool cnrSchemaInit(Schema* schema) {
  Property* property;
  Type* type;

  if (!cnSchemaInitDefault(schema)) cnFailTo(FAIL);

  // Item type. Say it's the size of a player, since they are bigger than balls.
  // TODO Can I really support multiple types at present??? What's best?
  if (!(type = cnTypeCreate("Item", sizeof(Player)))) cnFailTo(FAIL);
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnFailTo(FAIL);

  // Location property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Location",
    offsetof(Item, location), 2
  )) cnFailTo(FAIL);

  // Team property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, Item::TypePlayer, schema->floatType, cnrFieldTypeEnum,
    // TODO By C++11 standards, this offsetof should be okay, but I don't want
    // TODO to turn off warnings generally, since that could mask legitimate
    // TODO uses. Find a different solution here?
    "Team", offsetof(Player, team), 1
  )) cnFailTo(FAIL);

  // Type property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, Item::TypeAny, schema->floatType, cnrFieldTypeEnum, "Type",
    offsetof(Item, type), 1
  )) cnFailTo(FAIL);

  // Good to go.
  return true;

  FAIL:
  cnSchemaDispose(schema);
  return false;
}


// Seems first times are 0 1, so 0 0 should be an okay bogus.
State::State(): newSession(false), subtime(0), time(0) {}


}
}
