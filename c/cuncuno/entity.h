#ifndef cuncuno_entity_h
#define cuncuno_entity_h


#include "core.h"


typedef struct cnBag {

  // TODO id?

  /**
   * List of void pointers here.
   */
  cnList entities;

  /**
   * Positive or negative bag.
   */
  cnBool label;

} cnBag;


struct cnProperty; typedef struct cnProperty cnProperty;
struct cnSchema; typedef struct cnSchema cnSchema;
struct cnType; typedef struct cnType cnType;

struct cnProperty {

  /**
   * If this is an array/list property, says how many there are.
   *
   * If this varies by entity, then it's not useful for building matrices.
   * Therefore, this is a fixed count. A varied length would need to be
   * provided by copying out a pointer here or something.
   */
  cnCount count;

  union {

    /**
     * For more abstract properties, as needed.
     */
    void* data;

    /**
     * For the common case of struct field access.
     */
    cnCount offset;

  };

  /**
   * If not null, call this before finishing generic disposal.
   */
  void (*dispose)(const cnProperty* property);

  void (*get)(const cnProperty* property, const void* entity, void* storage);

  cnString name;

  void (*put)(const cnProperty* property, void* entity, const void* value);

  cnType* type;

};


/**
 * Provides functions for accessing entity attributes.
 */
struct cnSchema {

  /**
   * Corresponds to cnFloat (double).
   *
   * Could be NULL if undefined for this schema.
   */
  cnType* floatType;

  cnList types;

};


struct cnType {

  cnString name;

  cnList properties;

  cnSchema* schema;

  cnCount size;

};


void cnBagDispose(cnBag* bag);


void cnBagInit(cnBag* bag);


/**
 * Just disposes of the name for now.
 *
 * Doesn't dispose of nor free the type. Types manage properties, not vice
 * versa.
 */
void cnPropertyDispose(cnProperty* property);


/**
 * Provides simple struct field access by offset.
 */
cnBool cnPropertyInitField(
  cnProperty* property, cnType* type, char* name, cnCount offset, cnCount count
);


/**
 * Disposes all contained types and properties, too.
 */
void cnSchemaDispose(cnSchema* schema);


/**
 * Provides just float (double) type for now.
 */
cnBool cnSchemaInitDefault(cnSchema* schema);


/**
 * Disposes of all contained properties, too. Leaves name empty, size 0, and
 * schema NULL.
 *
 * Doesn't dispose of the schema. Schemas manage types, not vice versa.
 */
void cnTypeDispose(cnType* type);


cnBool cnTypeInit(cnType* type, char* name, cnCount size);


#endif
