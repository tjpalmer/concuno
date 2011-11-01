#include <stddef.h>
#include <string.h>
#include "domain.h"

using namespace concuno;


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


struct cnrFieldInfo {

  cnrFieldType fieldType;

  cnrType itemType;

  Count offset;

};


void cnrGameDispose(cnrGame* game) {
  // Dispose states.
  cnListEachBegin(&game->states, cnrState, state) {
    cnrStateDispose(state);
  } cnEnd;

  // Dispose team names.
  cnListEachBegin(&game->teamNames, String, name) {
    name->dispose();
  } cnEnd;
}


void cnrItemInit(cnrItem* item, cnrType type) {
  item->location[0] = 0.0;
  item->location[1] = 0.0;
  item->type = type;
}


void cnrPlayerInit(cnrPlayer* player) {
  // Generic item stuff.
  cnrItemInit(&player->item, cnrTypePlayer);
  // No kick to start with.
  player->kickAngle = cnNaN();
  player->kickPower = cnNaN();
  // Actual players start from 1, so 0 is a good invalid value.
  player->index = 0;
  // On the other hand, I expect 0 and 1 both for teams, so use -1 for bogus.
  player->team = -1;
}


void cnrPropertyFieldGet(Property* property, Entity entity, void* storage) {
  cnrFieldInfo* info = (cnrFieldInfo*)property->data;
  cnrItem* item = reinterpret_cast<cnrItem*>(entity);
  if (info->itemType != cnrTypeAny && item->type != info->itemType) {
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
  cnrFieldInfo* info = (cnrFieldInfo*)property->data;
  cnrItem* item = reinterpret_cast<cnrItem*>(entity);
  if (info->itemType != cnrTypeAny && item->type != info->itemType) {
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
  Property* property, Type* containerType, cnrType itemType,
  Type* type, cnrFieldType fieldType,
  const char* name, Count offset, Count count
) {
  cnrFieldInfo* info;
  // Safety items first.
  property->name.init();
  property->dispose = NULL;
  // Failing things.
  if (!cnStringPushStr(&property->name, name)) cnFailTo(FAIL);
  if (!(
    info = reinterpret_cast<cnrFieldInfo*>(malloc(sizeof(struct cnrFieldInfo)))
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
  if (!(type = cnTypeCreate("Item", sizeof(cnrPlayer)))) cnFailTo(FAIL);
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnFailTo(FAIL);

  // Location property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Location",
    offsetof(struct cnrItem, location), 2
  )) cnFailTo(FAIL);

  // Team property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, cnrTypePlayer, schema->floatType, cnrFieldTypeEnum, "Team",
    offsetof(struct cnrPlayer, team), 1
  )) cnFailTo(FAIL);

  // Type property.
  if (!(
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties))
  )) cnFailTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, cnrTypeAny, schema->floatType, cnrFieldTypeEnum, "Type",
    offsetof(struct cnrItem, type), 1
  )) cnFailTo(FAIL);

  // Good to go.
  return true;

  FAIL:
  cnSchemaDispose(schema);
  return false;
}


void cnrStateDispose(cnrState* state) {
  cnrStateInit(state);
}


void cnrStateInit(cnrState* state) {
  cnrItemInit(&state->ball.item, cnrTypeBall);
  state->newSession = false;
  // Seems first times are 0 1, so 0 0 should be an okay bogus.
  state->subtime = 0;
  state->time = 0;
}
