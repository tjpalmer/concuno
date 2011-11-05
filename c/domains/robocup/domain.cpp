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
    const char* name, Count count
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


TypedFieldProperty::TypedFieldProperty(
    Type* containerType, Item::Type $itemType,
    Type* type, cnrFieldType $fieldType,
    const char* name, Count count
):
  FieldProperty(containerType, type, name, count),
  fieldType($fieldType), itemType($itemType)
{}


void TypedFieldProperty::get(Entity entity, void* storage) {
  Item* item = reinterpret_cast<Item*>(entity);
  char* fieldAddress = reinterpret_cast<char*>(field(entity));
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
    unsigned int* in = reinterpret_cast<unsigned int*>(fieldAddress);
    Float* out = reinterpret_cast<Float*>(storage);
    Float* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else if (fieldType == cnrFieldTypeInt) {
    // Out must be float.
    Int* in = reinterpret_cast<Int*>(fieldAddress);
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
  char* fieldAddress = reinterpret_cast<char*>(field(entity));
  if (itemType != Item::TypeAny && item->type != itemType) {
    // Types don't match. Nothing to do here.
    // TODO Any way to indicate error?
  } else if (fieldType == cnrFieldTypeEnum) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    unsigned int* out = reinterpret_cast<unsigned int*>(fieldAddress);
    unsigned int* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = (unsigned int)*in;
    }
  } else if (fieldType == cnrFieldTypeInt) {
    // In must be float.
    Float* in = reinterpret_cast<Float*>(value);
    Int* out = reinterpret_cast<Int*>(fieldAddress);
    Int* outEnd = out + count;
    for (; out < outEnd; in++, out++) {
      *out = (Int)*in;
    }
  } else {
    // Just copy.
    FieldProperty::put(entity, value);
  }
}


void schemaInit(Schema& schema) {
  // Item type. Say it's the size of a player, since they are bigger than balls.
  // TODO Can I really support multiple types at present??? What's best?
  Type* type = new Type(schema, "Item", sizeof(Player));
  pushOrDelete(*schema.types, type);

  // Location property.
  struct LocationProperty: FieldProperty {
    LocationProperty(Type* containerType, Type* type):
      FieldProperty(containerType, type, "Location", 2) {}
    virtual void* field(Entity entity) {
      return reinterpret_cast<Item*>(entity)->location;
    }
  };
  type->properties.push(new LocationProperty(type, schema.floatType));

  // Team property.
  struct TeamProperty: TypedFieldProperty {
    TeamProperty(Type* containerType, Type* type):
      TypedFieldProperty(
        // TODO Really this is a size_t and not an enum. Fix!
        // TODO A template approach is probably better than this, anyway.
        containerType, Item::TypePlayer, type, cnrFieldTypeEnum, "Team", 1) {}
    virtual void* field(Entity entity) {
      return &reinterpret_cast<Player*>(entity)->team;
    }
  };
  type->properties.push(new TeamProperty(type, schema.floatType));

  // Type property.
  struct TypeProperty: TypedFieldProperty {
    TypeProperty(Type* containerType, Type* type):
      TypedFieldProperty(
        containerType, Item::TypeAny, type, cnrFieldTypeEnum, "Type", 1) {}
    virtual void* field(Entity entity) {
      return &reinterpret_cast<Item*>(entity)->type;
    }
  };
}


// Seems first times are 0 1, so 0 0 should be an okay bogus.
State::State(): newSession(false), subtime(0), time(0) {}


}
}
