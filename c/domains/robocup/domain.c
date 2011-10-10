#include <stddef.h>
#include <string.h>
#include "domain.h"


typedef enum {

  /**
   * These are of type 'int'.
   */
  cnrFieldTypeEnum,

  /**
   * These are of type 'cnInt' (usually 'long').
   */
  cnrFieldTypeInt,

  /**
   * Type cnFloat.
   */
  cnrFieldTypeFloat,

} cnrFieldType;


typedef struct cnrFieldInfo {

  cnrFieldType fieldType;

  cnrType itemType;

  cnCount offset;

}* cnrFieldInfo;


void cnrGameDispose(cnrGame game) {
  // Dispose states.
  cnListEachBegin(&game->states, struct cnrState, state) {
    cnrStateDispose(state);
  } cnEnd;
  cnListDispose(&game->states);

  // Dispose team names.
  cnListEachBegin(&game->teamNames, cnString, name) {
    cnStringDispose(name);
  } cnEnd;
  cnListDispose(&game->teamNames);
}


void cnrGameInit(cnrGame game) {
  cnListInit(&game->states, sizeof(struct cnrState));
  cnListInit(&game->teamNames, sizeof(cnString));
}


void cnrItemInit(cnrItem item, cnrType type) {
  item->location[0] = 0.0;
  item->location[1] = 0.0;
  item->type = type;
}


void cnrPlayerInit(cnrPlayer player) {
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


void cnrPropertyFieldGet(cnProperty* property, cnEntity entity, void* storage) {
  cnrFieldInfo info = (cnrFieldInfo)property->data;
  cnrItem item = entity;
  if (info->itemType != cnrTypeAny && item->type != info->itemType) {
    // Types don't match. Give NaNs.
    // TODO If we were more explicit, could we be more efficient in learning?
    cnFloat* out = storage;
    cnFloat* outEnd = out + property->count;
    for (; out < outEnd; out++) {
      *((cnFloat*)storage) = cnNaN();
    }
  } else if (info->fieldType == cnrFieldTypeEnum) {
    // Out must be float.
    unsigned int* in = (unsigned int*)(((char*)entity) + property->offset);
    cnFloat* out = storage;
    cnFloat* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else if (info->fieldType == cnrFieldTypeInt) {
    // Out must be float.
    cnInt* in = (cnInt*)(((char*)entity) + property->offset);
    cnFloat* out = storage;
    cnFloat* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = *in;
    }
  } else {
    // Just copy.
    memcpy(
      storage,
      ((char*)entity) + property->offset,
      property->count * property->type->size
    );
  }
}


void cnrPropertyFieldPut(cnProperty* property, cnEntity entity, void* value) {
  cnrFieldInfo info = (cnrFieldInfo)property->data;
  cnrItem item = entity;
  if (info->itemType != cnrTypeAny && item->type != info->itemType) {
    // Types don't match. Nothing to do here.
    // TODO Any way to indicate error?
  } else if (info->fieldType == cnrFieldTypeEnum) {
    // In must be float.
    cnFloat* in = value;
    unsigned int* out = (unsigned int*)(((char*)entity) + property->offset);
    unsigned int* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = (unsigned int)*in;
    }
  } else if (info->fieldType == cnrFieldTypeInt) {
    // In must be float.
    cnFloat* in = value;
    cnInt* out = (cnInt*)(((char*)entity) + property->offset);
    cnInt* outEnd = out + property->count;
    for (; out < outEnd; in++, out++) {
      *out = (cnInt)*in;
    }
  } else {
    // Just copy.
    memcpy(
      ((char*)entity) + property->offset,
      value,
      property->count * property->type->size
    );
  }
}


void cnrPropertyFieldInfoDispose(cnProperty* property) {
  // TODO Should this be in entity.c? It seems somewhat generally useful.
  free(property->data);
}


cnBool cnrPropertyInitTypedField(
  cnProperty* property, cnType* containerType, cnrType itemType,
  cnType* type, cnrFieldType fieldType,
  char* name, cnCount offset, cnCount count
) {
  cnrFieldInfo info;
  // Safety items first.
  cnStringInit(&property->name);
  property->dispose = NULL;
  // Failing things.
  if (!cnStringPushStr(&property->name, name)) cnErrTo(FAIL);
  if (!(info = malloc(sizeof(struct cnrFieldInfo)))) {
    cnFailTo(FAIL, "Failed to allocate field info.");
  }
  info->fieldType = fieldType;
  info->itemType = itemType;
  info->offset = offset;
  // Easy things.
  property->containerType = containerType;
  property->count = count;
  property->dispose = cnrPropertyFieldInfoDispose;
  property->get = cnrPropertyFieldGet;
  property->offset = offset;
  property->put = cnrPropertyFieldPut;
  property->topology = cnTopologyEuclidean;
  property->type = type;
  return cnTrue;

  FAIL:
  cnPropertyDispose(property);
  return cnFalse;
}


cnBool cnrSchemaInit(cnSchema* schema) {
  cnProperty* property;
  cnType* type;

  if (!cnSchemaInitDefault(schema)) cnErrTo(FAIL);

  // Item type. Say it's the size of a player, since they are bigger than balls.
  // TODO Can I really support multiple types at present??? What's best?
  if (!(type = cnTypeCreate("Item", sizeof(cnrPlayer)))) cnErrTo(FAIL);
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnErrTo(FAIL);

  // Location property.
  if (!(property = cnListExpand(&type->properties))) cnErrTo(FAIL);
  if (!cnPropertyInitField(
    property, type, schema->floatType, "Location",
    offsetof(struct cnrItem, location), 2
  )) cnErrTo(FAIL);

  // Team property.
  if (!(property = cnListExpand(&type->properties))) cnErrTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, cnrTypePlayer, schema->floatType, cnrFieldTypeEnum, "Team",
    offsetof(struct cnrPlayer, team), 1
  )) cnErrTo(FAIL);

  // Type property.
  if (!(property = cnListExpand(&type->properties))) cnErrTo(FAIL);
  if (!cnrPropertyInitTypedField(
    property, type, cnrTypeAny, schema->floatType, cnrFieldTypeInt, "Type",
    offsetof(struct cnrItem, type), 1
  )) cnErrTo(FAIL);

  // Good to go.
  return cnTrue;

  FAIL:
  cnSchemaDispose(schema);
  return cnFalse;
}


void cnrStateDispose(cnrState state) {
  cnListDispose(&state->players);
  cnrStateInit(state);
}


void cnrStateInit(cnrState state) {
  cnrItemInit(&state->ball.item, cnrTypeBall);
  state->newSession = cnFalse;
  cnListInit(&state->players, sizeof(struct cnrPlayer));
  // Seems first times are 0 1, so 0 0 should be an okay bogus.
  state->subtime = 0;
  state->time = 0;
}
