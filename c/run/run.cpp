#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "run.h"

using namespace concuno;


/**
 * For ultra simple pointers into structs, saying where to put data and what
 * type goes there (including the size in bytes).
 */
typedef struct cnvTypedOffset {

  /**
   * Offset in bytes.
   */
  Count offset;

  /**
   * Type at this offset.
   */
  Type* type;

} cnvTypedOffset;


/**
 * Fills in the bags with the given label information and data.
 */
bool cnvBuildBags(
  cnList(Bag)* bags,
  char* labelId, Type* labelType, cnListAny* labels,
  Type* featureType, cnListAny* features
);


/**
 * Returns the created item type or null for failure.
 */
Type* cnvLoadTable(
  const char* fileName, const char* typeName, Schema* schema, cnListAny* items
);


bool cnvPickFunctions(cnList(EntityFunction*)* functions, Type* type);


void cnvPrintType(Type* type);


bool cnvPushOrExpandProperty(
  Type* type, char* propertyName, cnvTypedOffset* offset
);


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;
  cnList(Bag) bags;
  cnListAny features;
  Type* featureType;
  cnList(EntityFunction*) functions;
  cnListAny labels;
  Type* labelType;
  RootNode* learnedTree = NULL;
  Learner learner;
  Schema schema;

  // Init lists first for safety.
  cnListInit(&bags, sizeof(Bag));
  // We don't yet know how big the items are.
  cnListInit(&features, 0);
  cnListInit(&functions, sizeof(EntityFunction*));
  cnListInit(&labels, 0);
  if (!cnSchemaInitDefault(&schema)) cnErrTo(DONE, "Init failed.");

  if (argc < 4) cnErrTo(
    DONE, "Usage: %s <features-file> <labels-file> <label-id>", argv[0]
  );

  // Load all the data.
  featureType = cnvLoadTable(argv[1], "Feature", &schema, &features);
  if (!featureType) cnErrTo(DONE, "Failed feature load.");
  labelType = cnvLoadTable(argv[2], "Label", &schema, &labels);
  if (!labelType) cnErrTo(DONE, "Failed label load.");

  // Build labeled bags.
  if (
    !cnvBuildBags(&bags, argv[3], labelType, &labels, featureType, &features)
  ) cnErrTo(DONE, "No bags.");
  // Choose some functions. TODO How to specify which??
  if (!cnvPickFunctions(&functions, featureType)) {
    cnErrTo(DONE, "No functions.");
  }
  printf("\n");

  // Learn something.
  cnListShuffle(&bags);
  learner.bags = &bags;
  learner.entityFunctions = &functions;
  learnedTree = learner.learnTree();
  if (!learnedTree) cnErrTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, stdout);
  printf("\n");

  result = EXIT_SUCCESS;

  DONE:
  cnNodeDrop(&learnedTree->node);
  cnSchemaDispose(&schema);
  cnListDispose(&labels);
  cnListEachBegin(&functions, EntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  cnListDispose(&functions);
  cnListDispose(&features);
  cnBagListDispose(&bags, NULL);
  return result;
}


bool cnvBuildBags(
  cnList(Bag)* bags,
  char* labelId, Type* labelType, cnListAny* labels,
  Type* featureType, cnListAny* features
) {
  char* feature = reinterpret_cast<char*>(features->items);
  char* featuresEnd = reinterpret_cast<char*>(cnListEnd(features));
  Count labelOffset = -1;
  bool result = false;

  // Find the offset matching the label id.
  cnListEachBegin(&labelType->properties, Property, property) {
    if (!strcmp(labelId, cnStr(&property->name))) {
      labelOffset = property->offset;
      printf("Label %s at offset %ld.\n", labelId, labelOffset);
      break;
    }
  } cnEnd;
  if (labelOffset < 0) cnErrTo(DONE, "Label %s not found.", labelId);

  // Assume for now that the labels and features go in the same order.
  cnListEachBegin(labels, char, labelItem) {
    Bag* bag = reinterpret_cast<Bag*>(cnListExpand(bags));
    Index bagId = *(Float*)labelItem;
    // Check the bag allocation, and init the thing.
    if (!bag) cnErrTo(DONE, "No bag.");
    bag->init();
    bag->label = *(Float*)(labelItem + labelOffset);
    // Add features.
    //printf("Reached bag %ld, labeled %u.\n", bagId, bag->label);
    for (; feature < featuresEnd; feature += features->itemSize) {
      Index featureBagId = *(Float*)feature;
      if (featureBagId != bagId) break;
      // The feature goes with this bag.
      cnListPush(bag->entities, &feature);
    }
  } cnEnd;

  // We winned!
  result = true;

  DONE:
  return result;
}


Type* cnvLoadTable(
  const char* fileName, const char* typeName, Schema* schema, cnListAny* items
) {
  Count countRead;
  FILE* file = NULL;
  String line;
  char* remaining;
  cnList(cnvTypedOffset) offsets;
  cnvTypedOffset* offsetsEnd;
  Type* type = NULL;

  // Init lists for safety.
  cnStringInit(&line);
  cnListInit(&offsets, sizeof(cnvTypedOffset));

  // Create the type.
  if (!(type = cnTypeCreate(typeName, 0))) cnErrTo(FAIL, "No type create.");
  type->schema = schema;
  if (!cnListPush(&schema->types, &type)) cnErrTo(FAIL, "No type for schema.");

  // Open the file.
  file = fopen(fileName, "r");
  if (!file) cnErrTo(FAIL, "Couldn't open '%s'.", fileName);

  // Read the headers.
  if (cnReadLine(file, &line) <= 0) cnErrTo(FAIL, "No headers.");
  remaining = cnStr(&line);
  while (true) {
    cnvTypedOffset* offset;
    char* next = cnParseStr(remaining, &remaining);
    if (!*next) break;
    if (next == (char*)line.items && *next == '%') {
      // Strip % prefix, used there for Matlab convenience.
      next++;
    }
    if (!(offset = reinterpret_cast<cnvTypedOffset*>(cnListExpand(&offsets)))) {
      cnErrTo(FAIL, "No offset.");
    }
    if (!cnvPushOrExpandProperty(type, next, offset)) {
      cnErrTo(FAIL, "Property stuck.");
    }
  }
  // Now that we have the full type, it's stable.
  items->itemSize = type->size;
  offsetsEnd = reinterpret_cast<cnvTypedOffset*>(cnListEnd(&offsets));
  // Report it for now, too.
  cnvPrintType(type);

  // Read the lines. Each is a separate item.
  while ((countRead = cnReadLine(file, &line)) > 0) {
    Entity item;
    // Allocate then read the fields on this line.
    if (!(item = cnListExpand(items))) cnErrTo(FAIL, "No item.");
    remaining = cnStr(&line);
    cnListEachBegin(&offsets, cnvTypedOffset, offset) {
      // TODO Don't assume all are floats! Need custom parsing?
      char* former = remaining;
      Float value = strtod(remaining, &remaining);
      if (remaining == former) cnErrTo(FAIL, "No data to read?");
      *(Float*)(((char*)item) + offset->offset) = value;
    } cnEnd;
  }
  if (countRead < 0) cnErrTo(FAIL, "Error reading line.");

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


bool cnvPickFunctions(cnList(EntityFunction*)* functions, Type* type) {
  // For now, just put in valid and common functions for each property.
  if (!cnPushValidFunction(functions, type->schema, 1)) {
    cnErrTo(FAIL, "No valid 1.");
  }
  if (!cnPushValidFunction(functions, type->schema, 2)) {
    cnErrTo(FAIL, "No valid 2.");
  }
  cnListEachBegin(&type->properties, Property, property) {
    EntityFunction* function;
    if (property == type->properties.items) {
      // Skip the first (the bag id).
      continue;
    }
    if (!(function = cnEntityFunctionCreateProperty(property))) {
      cnErrTo(FAIL, "No function.");
    }
    if (!cnListPush(functions, &function)) {
      cnEntityFunctionDrop(function);
      cnErrTo(FAIL, "Function not pushed.");
    }
    // TODO Distance (and difference?) angle, too?
    if (true || !strcmp("Location", cnStr(&function->name))) {
      EntityFunction* distance;
      if (true) {
        // Actually, skip this N^2 thing for now. For many items per bag and few
        // bags, this is both extremely slow and allows overfit, since there are
        // so many options to consider.
        //continue;
      }

      // Distance.
      if (!(distance = cnEntityFunctionCreateDistance(function))) {
        cnErrTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnErrTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }

      // Difference.
      if (!(distance = cnEntityFunctionCreateDifference(function))) {
        cnErrTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnErrTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }
    }
  } cnEnd;

  // We winned!
  return true;

  FAIL:
  // TODO Remove all added functions?
  return false;
}


void cnvPrintType(Type* type) {
  printf("type %s\n", cnStr(&type->name));
  cnListEachBegin(&type->properties, Property, property) {
    printf(
      "  var %s: %s[%ld]\n",
      cnStr(&property->name), cnStr(&property->type->name), property->count
    );
  } cnEnd;
  // TODO Audit type size?
}


bool cnvPushOrExpandProperty(
  Type* type, char* propertyName, cnvTypedOffset* offset
) {
  Property* property = NULL;
  bool result = false;

  // Find the property with this name.
  cnListEachBegin(&type->properties, Property, propertyToCheck) {
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
    property = reinterpret_cast<Property*>(cnListExpand(&type->properties));
    if (!property) cnErrTo(DONE, "Failed to alloc %s property.", propertyName);
    // TODO Assumed always float for now.
    if (!cnPropertyInitField(
      property, type, type->schema->floatType, propertyName, type->size, 1
    )) cnErrTo(DONE, "Failed to init %s property.", propertyName);
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
  result = true;

  DONE:
  return result;
}
