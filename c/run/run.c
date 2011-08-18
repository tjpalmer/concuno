#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "run.h"


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


cnBool cnvPickFunctions(cnList(cnEntityFunction*)* functions, cnType* type);


void cnvPrintType(cnType* type);


cnBool cnvPushOrExpandProperty(
  cnType* type, char* propertyName, cnvTypedOffset* offset
);


int main(int argc, char** argv) {
  int result = EXIT_FAILURE;
  cnList(cnBag) bags;
  cnListAny features;
  cnType* featureType;
  cnList(cnEntityFunction*) functions;
  cnBool initOkay = cnTrue;
  cnListAny labels;
  cnType* labelType;
  cnRootNode* learnedTree = NULL;
  cnLearner learner;
  cnSchema schema;

  // Init lists first for safety.
  cnListInit(&bags, sizeof(cnBag));
  // We don't yet know how big the items are.
  cnListInit(&features, 0);
  cnListInit(&functions, sizeof(cnEntityFunction*));
  cnListInit(&labels, 0);
  initOkay &= cnSchemaInitDefault(&schema);
  initOkay &= cnLearnerInit(&learner, NULL);
  if (!initOkay) cnFailTo(DONE, "Init failed.");

  if (argc < 4) cnFailTo(
    DONE, "Usage: %s <features-file> <labels-file> <label-id>", argv[0]
  );

  // Load all the data.
  featureType = cnvLoadTable(argv[1], "Feature", &schema, &features);
  if (!featureType) cnFailTo(DONE, "Failed feature load.");
  labelType = cnvLoadTable(argv[2], "Label", &schema, &labels);
  if (!labelType) cnFailTo(DONE, "Failed label load.");

  // Build labeled bags.
  if (
    !cnvBuildBags(&bags, argv[3], labelType, &labels, featureType, &features)
  ) cnFailTo(DONE, "No bags.");
  // Choose some functions. TODO How to specify which??
  if (!cnvPickFunctions(&functions, featureType)) {
    cnFailTo(DONE, "No functions.");
  }
  printf("\n");

  // Learn something.
  if (!cnLearnerInit(&learner, NULL)) cnFailTo(DONE, "No learner.");
  cnListShuffle(&bags);
  learner.bags = &bags;
  learner.entityFunctions = &functions;
  learnedTree = cnLearnTree(&learner);
  if (!learnedTree) cnFailTo(DONE, "No learned tree.");

  // Display the learned tree.
  printf("\n");
  cnTreeWrite(learnedTree, stdout);
  printf("\n");

  result = EXIT_SUCCESS;

  DONE:
  cnNodeDrop(&learnedTree->node);
  cnLearnerDispose(&learner);
  cnSchemaDispose(&schema);
  cnListDispose(&labels);
  cnListEachBegin(&functions, cnEntityFunction*, function) {
    cnEntityFunctionDrop(*function);
  } cnEnd;
  cnListDispose(&functions);
  cnListDispose(&features);
  cnListEachBegin(&bags, cnBag, bag) {
    cnBagDispose(bag);
  } cnEnd;
  cnListDispose(&bags);
  return result;
}


cnBool cnvBuildBags(
  cnList(cnBag)* bags,
  char* labelId, cnType* labelType, cnListAny* labels,
  cnType* featureType, cnListAny* features
) {
  char* feature = features->items;
  char* featuresEnd = cnListEnd(features);
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

  // Assume for now that the labels and features go in the same order.
  cnListEachBegin(labels, char, labelItem) {
    cnBag* bag = cnListExpand(bags);
    cnIndex bagId = *(cnFloat*)labelItem;
    // Check the bag allocation, and init the thing.
    if (!bag) cnFailTo(DONE, "No bag.");
    if (!cnBagInit(bag)) cnFailTo(DONE, "No bag init.");
    bag->label = *(cnFloat*)(labelItem + labelOffset);
    // Add features.
    //printf("Reached bag %ld, labeled %u.\n", bagId, bag->label);
    for (; feature < featuresEnd; feature += features->itemSize) {
      cnIndex featureBagId = *(cnFloat*)feature;
      if (featureBagId != bagId) break;
      // The feature goes with this bag.
      cnListPush(bag->entities, &feature);
    }
  } cnEnd;

  // We winned!
  result = cnTrue;

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


cnBool cnvPickFunctions(cnList(cnEntityFunction*)* functions, cnType* type) {
  // For now, just put in one identity function for each property.
  cnListEachBegin(&type->properties, cnProperty, property) {
    cnEntityFunction* function;
    if (property == type->properties.items) {
      // Skip the first (the bag id).
      continue;
    }
    if (!(function = cnEntityFunctionCreateProperty(property))) {
      cnFailTo(FAIL, "No function.");
    }
    if (!cnListPush(functions, &function)) {
      cnEntityFunctionDrop(function);
      cnFailTo(FAIL, "Function not pushed.");
    }
    // TODO Distance (and difference?) angle, too?
    if (cnTrue || !strcmp("Location", cnStr(&function->name))) {
      cnEntityFunction* distance;
      if (cnTrue) {
        // Actually, skip this N^2 thing for now. For many items per bag and few
        // bags, this is both extremely slow and allows overfit, since there are
        // so many options to consider.
        //continue;
      }

      // Distance.
      if (!(distance = cnEntityFunctionCreateDistance(function))) {
        cnFailTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnFailTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }

      // Difference.
      if (!(distance = cnEntityFunctionCreateDifference(function))) {
        cnFailTo(FAIL, "No distance %s.", cnStr(&function->name));
      }
      if (!cnListPush(functions, &distance)) {
        cnEntityFunctionDrop(distance);
        cnFailTo(FAIL, "Function %s not pushed.", cnStr(&distance->name));
      }
    }
  } cnEnd;

  // We winned!
  return cnTrue;

  FAIL:
  // TODO Remove all added functions?
  return cnFalse;
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
