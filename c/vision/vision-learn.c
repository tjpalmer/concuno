#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vision-learn.h"


/**
 * For ultra simple pointers into structs, saying where to put data and what
 * type goes there (including the size in bytes).
 */
typedef struct cnvTypedOffset {

  /**
   * Offset in bytes.
   */
  cnCount offset;

  /**
   * Type at this offset.
   */
  cnType* type;

} cnvTypedOffset;


cnBool cnvLoadBags(char* indicatorsName, char* featuresName);


cnBool cnvLoadGroupedMatrix(char* name);


void cnvPrintType(cnType* type);


cnBool cnvPushOrExpandProperty(
  cnType* type, char* propertyName, cnvTypedOffset* offset
);


/**
 * Returns the created entity type or null for failure.
 */
cnType* cnvSchemaInit(cnSchema* schema);


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;

  if (argc < 3) cnFailTo(DONE, "Tell me which two files to open!");
  cnvLoadBags(argv[1], argv[2]);
  result = EXIT_SUCCESS;

  DONE:
  return result;
}


cnBool cnvLoadBags(char* indicatorsName, char* featuresName) {
  cnBool result = cnFalse;
  cnvLoadGroupedMatrix(featuresName);
  return result;
}


cnBool cnvLoadGroupedMatrix(char* name) {
  cnBool result = cnFalse;
  cnCount countRead;
  FILE* file = NULL;
  cnListAny items;
  cnString line;
  char* remaining;
  cnList(cnvTypedOffset) offsets;
  cnvTypedOffset* offsetsEnd;
  cnSchema schema;
  cnType* type;

  // Init the data.
  // No size yet for items.
  cnListInit(&items, 0);
  cnStringInit(&line);
  cnListInit(&offsets, sizeof(cnString));
  if (!(type = cnvSchemaInit(&schema))) cnFailTo(DONE, "No schema.");

  // Open the file.
  file = fopen(name, "r");
  if (!file) cnFailTo(DONE, "Couldn't open '%s'.", name);

  // Read the headers.
  if (cnReadLine(file, &line) <= 0) cnFailTo(DONE, "No headers.");
  remaining = cnStr(&line);
  while (cnTrue) {
    cnvTypedOffset* offset;
    char* next = cnParseStr(remaining, &remaining);
    if (!*next) break;
    if (next == (char*)line.items) continue; // File group name first. Ignored.
    if (!(offset = cnListExpand(&offsets))) cnFailTo(DONE, "No offset.");
    if (!cnvPushOrExpandProperty(type, next, offset)) {
      cnFailTo(DONE, "Property stuck.");
    }
  }
  // Now that we have the full type, it's stable.
  items.itemSize = type->size;
  offsetsEnd = cnListEnd(&offsets);
  // Report it for now, too.
  cnvPrintType(type);

  // Read the data. First column tells us what bag we are in. Assume bags that
  // aren't intermixed.
  while ((countRead = cnReadLine(file, &line)) > 0) {
    cnIndex bagId;
    cnEntity item;
    cnvTypedOffset* offset = offsets.items;
    remaining = cnStr(&line);
    // Read the bag (image) id. TODO Step bag to bag.
    bagId = strtol(remaining, &remaining, 10);
    if (!(item = cnListExpand(&items))) cnFailTo(DONE, "No item.");
    // Now read the fields themselves.
    for (
      offset = offsets.items; offset < offsetsEnd; offset += offsets.itemSize
    ) {
      // TODO Don't assume all are floats! Need custom parsing?
      char* former = remaining;
      cnFloat value = strtod(remaining, &remaining);
      if (remaining == former) cnFailTo(DONE, "No data to read?");
      *(cnFloat*)(((char*)item) + offset->offset) = value;
    }
  }
  if (countRead < 0) cnFailTo(DONE, "Error reading line.");

  DONE:
  cnSchemaDispose(&schema);
  cnListDispose(&offsets);
  cnStringDispose(&line);
  cnListDispose(&items);
  if (file) fclose(file);
  return result;
}


void cnvPrintType(cnType* type) {
  printf("type %s\n", cnStr(&type->name));
  cnListEachBegin(&type->properties, cnProperty, property) {
    printf(
      "  var %s: %s[%ld]\n",
      cnStr(&property->name), cnStr(&property->type->name), property->count
    );
  } cnEnd;
  // TODO Audit type size?
}


cnBool cnvPushOrExpandProperty(
  cnType* type, char* propertyName, cnvTypedOffset* offset
) {
  cnProperty* property = NULL;
  cnBool result = cnFalse;

  // Find the property with this name.
  cnListEachBegin(&type->properties, cnProperty, propertyToCheck) {
    if (property) {
      // Found a previous property, so offset this following one.
      propertyToCheck->offset += property->type->size;
    } else if (!strcmp(cnStr(&propertyToCheck->name), propertyName)) {
      // Found it, so remember and expand it.
      property = propertyToCheck;
      property->count++;
    }
  } cnEnd;

  if (!property) {
    // Didn't find anything, so push a new property.
    property = cnListExpand(&type->properties);
    if (!property) cnFailTo(DONE, "Failed to alloc %s property.", propertyName);
    // TODO Assumed always float for now.
    if (!cnPropertyInitField(
      property, type, type->schema->floatType, propertyName, type->size, 1
    )) cnFailTo(DONE, "Failed to init %s property.", propertyName);
  }

  // Update the type's overall size, too.
  type->size += property->type->size;
  // If requested, say where and what type this new slot is.
  if (offset) {
    // It's at the last index of the property (count - 1).
    offset->offset =
      property->offset + (property->count - 1) * property->type->size;
    offset->type = property->type;
  }
  // We winned.
  result = cnTrue;

  DONE:
  return result;
}


cnType* cnvSchemaInit(cnSchema* schema) {
  cnType* type = NULL;

  if (!cnSchemaInitDefault(schema)) cnFailTo(DONE, "No default schema.");
  if (!(type = cnListExpand(&schema->types))) {
    cnFailTo(FAIL, "No type for schema.");
  }
  if (!cnTypeInit(type, "Entity", 0)) { // What size, really?
    type = NULL;
    cnFailTo(FAIL, "No type init.");
  }
  type->schema = schema;

  // We winned! Skip fail, too.
  goto DONE;

  FAIL:
  cnSchemaDispose(schema);

  DONE:
  return type;
}
