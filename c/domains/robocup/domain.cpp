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


struct TypedFieldProperty: FieldProperty {

  TypedFieldProperty(
    Type* containerType, Item::Type itemType,
    Type* type, cnrFieldType fieldType,
    const char* name, Count offset, Count count
  );

  virtual void get(Entity entity, void* storage);

  virtual void put(Entity entity, void* value);

  cnrFieldType fieldType;

  Item::Type itemType;

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


void TypedFieldProperty::get(Entity entity, void* storage) {
  Item* item = reinterpret_cast<Item*>(entity);
  if (itemType != Item::TypeAny && item->type != itemType) {
    // Types don't match. Give NaNs.
    // TODO If we were more explicit, could we be more efficient in learning?
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + count;
    for (; out < outEnd; out++) {
      *((Float*)storage) = cnNaN();
    }
  } else if (fieldType == cnrFieldTypeEnum) {
    // Out must be float.
    // TODO Make each of these cases a different subtype of Property?
    unsigned int* in = (unsigned int*)(((char*)entity) + offset);
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else if (fieldType == cnrFieldTypeInt) {
    // Out must be float.
    Int* in = (Int*)(((char*)entity) + offset);
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else {
    // Just copy.
    FieldProperty::get(entity, storage);
  }
}


void TypedFieldProperty::put(Entity entity, void* value) {
  Item* item = reinterpret_cast<Item*>(entity);
  if (itemType != Item::TypeAny && item->type != itemType) {
    // Types don't match. Nothing to do here.
    // TODO Any way to indicate error?
  } else if (fieldType == cnrFieldTypeEnum) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    unsigned int* out = (unsigned int*)(((char*)entity) + offset);
    unsigned int* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = (unsigned int)*in;
    }
  } else if (fieldType == cnrFieldTypeInt) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    Int* out = (Int*)(((char*)entity) + offset);
    Int* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = (Int)*in;
    }
  } else {
    // Just copy.
    FieldProperty::put(entity, value);
  }
}


TypedFieldProperty::TypedFieldProperty(
    Type* containerType, Item::Type $itemType,
    Type* type, cnrFieldType $fieldType,
    const char* name, Count offset, Count count
):
  FieldProperty(containerType, type, name, offset, count),
  fieldType($fieldType), itemType($itemType)
{}


bool cnrSchemaInit(Schema* schema) {
  cnSchemaInitDefault(schema);

  // Item type. Say it's the size of a player, since they are bigger than balls.
  // TODO Can I really support multiple types at present??? What's best?
  Type* type = new Type("Item", sizeof(Player));
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnFailTo(FAIL);

  // Location property.
  type->properties.push(new FieldProperty(
    type, schema->floatType, "Location", offsetof(Item, location), 2
  ));

  // Team property.
  type->properties.push(new TypedFieldProperty(
    type, Item::TypePlayer, schema->floatType, cnrFieldTypeEnum, "Team",
    // TODO By C++11 standards, this offsetof should be okay, but I don't want
    // TODO to turn off warnings generally, since that could mask legitimate
    // TODO uses. Find a different solution here?
    offsetof(Player, team), 1
  ));

  // Type property.
  type->properties.push(new TypedFieldProperty(
    type, Item::TypeAny, schema->floatType, cnrFieldTypeEnum, "Type",
    offsetof(Item, type), 1
  ));

  // Good to go.
  return true;

  FAIL:
  return false;
}


// Seems first times are 0 1, so 0 0 should be an okay bogus.
State::State(): newSession(false), subtime(0), time(0) {}


}
}
