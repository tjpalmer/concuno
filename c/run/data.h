#ifndef concuno_run_data_h
#define concuno_run_data_h


#include <concuno.h>


namespace concuno {namespace run {


/**
 * For ultra simple pointers into structs, saying where to put data and what
 * type goes there (including the size in bytes).
 */
struct TypedOffset {

  /**
   * Offset in bytes.
   */
  Count offset;

  /**
   * Type at this offset.
   */
  Type* type;

};


/**
 * Fills in the bags with the given label information and data.
 */
bool buildBags(
  List<Bag>* bags,
  const std::string& labelId, Type* labelType, ListAny* labels,
  Type* featureType, ListAny* features
);


/**
 * Returns the created item type.
 */
Type* loadTable(
  const std::string& fileName, const std::string& typeName, Schema& schema,
  ListAny* items
);


void pickFunctions(std::vector<EntityFunction*>& functions, Type* type);


void printType(Type* type);


void pushOrExpandProperty(
  Type* type, const std::string& propertyName, TypedOffset* offset
);


}}


#endif
