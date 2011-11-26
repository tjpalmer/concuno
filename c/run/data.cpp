#include <fstream>
#include "data.h"

using namespace std;


namespace concuno {namespace run {


bool buildBags(
  List<Bag>* bags,
  const string& labelId, Type* labelType, ListAny* labels,
  Type* featureType, ListAny* features
) {
  char* feature = reinterpret_cast<char*>(features->items);
  char* featuresEnd = reinterpret_cast<char*>(cnListEnd(features));
  Count labelOffset = -1;
  bool result = false;

  // Find the offset matching the label id.
  for (size_t p = 0; p < labelType->properties->size(); p++) {
    OffsetProperty* property =
      dynamic_cast<OffsetProperty*>(labelType->properties[p]);
    if (property->name == labelId) {
      labelOffset = property->offset;
      printf("Label %s at offset %ld.\n", labelId.c_str(), labelOffset);
      break;
    }
  }
  if (labelOffset < 0) cnErrTo(DONE, "Label %s not found.", labelId.c_str());

  // Assume for now that the labels and features go in the same order.
  cnListEachBegin(labels, char, labelItem) {
    Bag* bag = reinterpret_cast<Bag*>(cnListExpand(bags));
    Index bagId = *(Float*)labelItem;
    // Check the bag allocation, and init the thing.
    if (!bag) cnErrTo(DONE, "No bag.");
    new(bag) Bag;
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


Type* loadTable(
  const string& fileName, const string& typeName, Schema& schema,
  ListAny* items
) {
  List<TypedOffset> offsets;
  TypedOffset* offsetsEnd;

  // Create the type.
  Type* type = new Type(schema, typeName.c_str(), 0);
  pushOrDelete(*schema.types, type);

  // Open the file.
  ifstream file(fileName.c_str());
  // TODO Need to check is_open?
  if (!file) throw Error(Buf() << "Couldn't open: " << fileName);

  // Read the headers.
  string line;
  if (!getline(file, line)) throw Error("No headers.");
  stringstream remaining(line);
  string next;
  while (remaining >> next) {
    TypedOffset* offset;
    if (type->properties->empty() && next[0] == '%') {
      // Strip % prefix, used there for Matlab convenience.
      next.erase(0, 1);
    }
    if (!(offset = reinterpret_cast<TypedOffset*>(cnListExpand(&offsets)))) {
      throw Error("No offset.");
    }
    pushOrExpandProperty(type, next, offset);
  }
  // Now that we have the full type, it's stable.
  items->itemSize = type->size;
  offsetsEnd = reinterpret_cast<TypedOffset*>(cnListEnd(&offsets));
  // Report it for now, too.
  printType(type);

  // Read the lines. Each is a separate item.
  Count i = 0;
  while (getline(file, line)) {
    i++;
    Entity item;
    // Allocate then read the fields on this line.
    if (!(item = cnListExpand(items))) throw Error("No item.");
    // For some reason, reuse with remaining.str(line) leaves fail/bad bits.
    stringstream remaining(line);
    cnListEachBegin(&offsets, TypedOffset, offset) {
      // TODO Don't assume all are floats! Need custom parsing?
      Float value;
      if (!(remaining >> value)) throw Error("No data to read?");
      *(Float*)(((char*)item) + offset->offset) = value;
    } cnEnd;
  }
  if (file.bad() || !file.eof()) throw Error("Error reading line.");

  // We winned!
  return type;
}


void pickFunctions(std::vector<EntityFunction*>& functions, Type* type) {
  // For now, just put in valid and common functions for each property.
  (new ValidityEntityFunction(*type->schema, 1))->pushOrDelete(functions);
  (new ValidityEntityFunction(*type->schema, 2))->pushOrDelete(functions);
  // Loop on all but the first (the bag id).
  for (size_t p = 1; p < type->properties->size(); p++) {
    Property& property = *type->properties[p];
    EntityFunction* function = new PropertyEntityFunction(property);
    function->pushOrDelete(functions);
    // TODO Distance (and difference?) angle, too?
    if (true || function->name == "Location") {
      if (true) {
        // Actually, skip this N^2 thing for now. For many items per bag and few
        // bags, this is both extremely slow and allows overfit, since there are
        // so many options to consider.
        //continue;
      }
      // Distance and difference.
      (new DistanceEntityFunction(*function))->pushOrDelete(functions);
      (new DifferenceEntityFunction(*function))->pushOrDelete(functions);
    }
  }
}


void printType(Type* type) {
  printf("type %s\n", type->name.c_str());
  for (size_t p = 1; p < type->properties->size(); p++) {
    Property* property = type->properties[p];
    printf(
      "  var %s: %s[%ld]\n",
      property->name.c_str(), property->type->name.c_str(), property->count
    );
  }
  // TODO Audit type size?
}


void pushOrExpandProperty(
  Type* type, const std::string& propertyName, TypedOffset* offset
) {
  OffsetProperty* property = NULL;

  // Find the property with this name.
  for (size_t p = 1; p < type->properties->size(); p++) {
    OffsetProperty* propertyToCheck =
      dynamic_cast<OffsetProperty*>(type->properties[p]);
    if (property) {
      // Found a previous property, so offset this following one.
      propertyToCheck->offset += property->type->size;
    } else if (propertyToCheck->name == propertyName) {
      // Found it, so remember and expand it.
      property = propertyToCheck;
      property->count++;
    }
  }

  if (!property) {
    // Didn't find anything, so push a new property.
    property = new OffsetProperty(
      type, type->schema->floatType, propertyName.c_str(), type->size, 1
    );
    type->properties.push(property);
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
}


}}
