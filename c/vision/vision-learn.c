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


/**
 * Fills in the bags with the given label information and data.
 */
cnBool cnvBuildBags(
  cnList(cnBag)* bags,
  char* labelId, cnType* labelType, cnListAny* labels,
  cnType* featureType, cnListAny* features
);


/**
 * Returns the created item type or null for failure.
 */
cnType* cnvLoadTable(
  char* fileName, char* typeName, cnSchema* schema, cnListAny* items
);


void cnvPrintType(cnType* type);


cnBool cnvPushOrExpandProperty(
  cnType* type, char* propertyName, cnvTypedOffset* offset
);


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;
  cnList(cnBag) bags;
  cnListAny features;
  cnType* featureType;
  cnListAny labels;
  cnType* labelType;
  cnSchema schema;

  // Init lists first for safety.
  // We don't yet know how big the items are.
  cnListInit(&features, 0);
  cnListInit(&bags, sizeof(cnBag));
  cnListInit(&labels, 0);
  if (!cnSchemaInitDefault(&schema)) cnFailTo(DONE, "No schema.");

  if (argc < 4) cnFailTo(
    DONE, "Usage: %s <label-id> <labels-file> <features-file>", argv[0]
  );

  // Load all the data.
  labelType = cnvLoadTable(argv[2], "Label", &schema, &labels);
  if (!labelType) cnFailTo(DONE, "Failed label load.");
  featureType = cnvLoadTable(argv[3], "Feature", &schema, &features);
  if (!featureType) cnFailTo(DONE, "Failed feature load.");

  // Build labeled bags.
  cnvBuildBags(&bags, argv[1], labelType, &labels, featureType, &features);

  // TODO Pick functions.
  // TODO Learn something.

  result = EXIT_SUCCESS;

  DONE:
  cnSchemaDispose(&schema);
  cnListDispose(&labels);
  cnListDispose(&features);
  cnListDispose(&bags);
  return result;
}


cnBool cnvBuildBags(
  cnList(cnBag)* bags,
  char* labelId, cnType* labelType, cnListAny* labels,
  cnType* featureType, cnListAny* features
) {
  cnIndex previousBagId = -1;
  cnBool label = cnFalse;
  char* labelItem = labels->items;
  char* labelItemsEnd = cnListEnd(labels);
  cnCount labelOffset = -1;
  cnBool result = cnFalse;

  // Find the offset matching the label id.
  cnListEachBegin(&labelType->properties, cnProperty, property) {
    if (!strcmp(labelId, cnStr(&property->name))) {
      labelOffset = property->offset;
      printf("Label %s at offset %ld.\n", labelId, labelOffset);
      break;
    }
  } cnEnd;
  if (labelOffset < 0) cnFailTo(DONE, "Label %s not found.", labelId);

  // Assume for now that the labels and items go in the same order.
  // Go to one before the first label item, so the first forward step is good.
  labelItem -= labels->itemSize;
  cnListEachBegin(features, void, feature) {
    // The bag id is always the first feature.
    cnIndex bagId = *(cnFloat*)feature;
    if (feature == features->items || bagId != previousBagId) {
      // New bag.
      cnIndex labelBagId;
      while (cnTrue) {
        labelItem += labels->itemSize;
        if (labelItem >= labelItemsEnd) {
          cnFailTo(DONE, "Never found labels for bag %ld.", bagId);
        }
        label = *(cnFloat*)(labelItem + labelOffset);
        labelBagId = *(cnFloat*)labelItem;
        if (bagId == labelBagId) {
          // Bags match. Move on.
          break;
        } else {
          // No items for this bag, but it has a label.
          printf("No items for bag %ld, labeled %u.\n", labelBagId, label);
          // TODO Push a bag anyway!
        }
      }
      printf("Reached bag %ld, labeled %u.\n", bagId, label);
      // Remember where we are.
      previousBagId = bagId;
    }
  } cnEnd;

  DONE:
  return result;
}


cnType* cnvLoadTable(
  char* fileName, char* typeName, cnSchema* schema, cnListAny* items
) {
  cnCount countRead;
  FILE* file = NULL;
  cnString line;
  char* remaining;
  cnList(cnvTypedOffset) offsets;
  cnvTypedOffset* offsetsEnd;
  cnType* type = NULL;

  // Init lists for safety.
  cnStringInit(&line);
  cnListInit(&offsets, sizeof(cnvTypedOffset));

  // Create the type.
  if (!(type = cnTypeCreate(typeName, 0))) cnFailTo(FAIL, "No type create.");
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnFailTo(FAIL, "No type for schema.");

  // Open the file.
  file = fopen(fileName, "r");
  if (!file) cnFailTo(FAIL, "Couldn't open '%s'.", fileName);

  // Read the headers.
  if (cnReadLine(file, &line) <= 0) cnFailTo(FAIL, "No headers.");
  remaining = cnStr(&line);
  while (cnTrue) {
    cnvTypedOffset* offset;
    char* next = cnParseStr(remaining, &remaining);
    if (!*next) break;
    if (next == (char*)line.items && *next == '%') {
      // Strip % prefix, used there for Matlab convenience.
      next++;
    }
    if (!(offset = cnListExpand(&offsets))) cnFailTo(FAIL, "No offset.");
    if (!cnvPushOrExpandProperty(type, next, offset)) {
      cnFailTo(FAIL, "Property stuck.");
    }
  }
  // Now that we have the full type, it's stable.
  items->itemSize = type->size;
  offsetsEnd = cnListEnd(&offsets);
  // Report it for now, too.
  cnvPrintType(type);

  // Read the lines. Each is a separate item.
  while ((countRead = cnReadLine(file, &line)) > 0) {
    cnEntity item;
    // Allocate then read the fields on this line.
    if (!(item = cnListExpand(items))) cnFailTo(FAIL, "No item.");
    remaining = cnStr(&line);
    cnListEachBegin(&offsets, cnvTypedOffset, offset) {
      // TODO Don't assume all are floats! Need custom parsing?
      char* former = remaining;
      cnFloat value = strtod(remaining, &remaining);
      if (remaining == former) cnFailTo(FAIL, "No data to read?");
      *(cnFloat*)(((char*)item) + offset->offset) = value;
    } cnEnd;
  }
  if (countRead < 0) cnFailTo(FAIL, "Error reading line.");

  // We winned!
  goto DONE;

  FAIL:
  if (type) {
    // Make the type go away, for tidiness.
    cnTypeDrop(type);
    schema->types.count--;
    type = NULL;
  }

  DONE:
  if (file) fclose(file);
  cnListDispose(&offsets);
  cnStringDispose(&line);
  return type;
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
