#ifndef cuncuno_entity_h
#define cuncuno_entity_h


#include "core.h"


typedef struct cnBag {

  // TODO id?

  /**
   * Pointers to entities, defined entirely by properties and entity functions.
   */
  cnList(void*) entities;

  /**
   * Positive or negative bag.
   */
  cnBool label;

} cnBag;


typedef enum {

  /**
   * Also called standard or real product topology or such like. Just presumed
   * open (to machine limits) R^N space here.
   */
  cnTopologyEuclidean,

  // TODO Use general SpecialOrthogonal and SpecialEuclidean across different
  // TODO dimensionalities?
  //cnTopologyCircle,

  // TODO The following is the power set, actually, but for discrete space
  // TODO allows what we want for discrete properties, I think.
  //cnTopologyDiscrete,

} cnTopology;


struct cnEntityFunction; typedef struct cnEntityFunction cnEntityFunction;
struct cnFunction; typedef struct cnFunction cnFunction;
struct cnPredicate; typedef struct cnPredicate cnPredicate;
struct cnProperty; typedef struct cnProperty cnProperty;
struct cnSchema; typedef struct cnSchema cnSchema;
struct cnType; typedef struct cnType cnType;


/**
 * Maps one or more entities to one or more values. Such functions should
 * always be organized in a context where the entity type is understood or
 * otherwise discernable by the function pointers here.
 */
struct cnEntityFunction {

  void* data;

  cnCount inCount;

  cnString name;

  cnCount outCount;

  cnTopology outTopology;

  cnType* outType;

  /**
   * If not null, call this before finishing generic disposal.
   */
  void (*dispose)(const cnEntityFunction* function);

  /**
   * Receives an array of pointers to entities, and provides an array of values
   * whose individual sizes are given by outType.
   */
  void (*get)(cnEntityFunction* function, void** ins, void* outs);

};


/**
 * Just a generic function with private data. The meaning depends on context.
 */
struct cnFunction {

  void* data;

  // TODO Name or types or anything?

  void (*dispose)(cnFunction* function);

  /**
   * Return value indicates status.
   */
  cnBool (*evaluate)(cnFunction* function, void* in, void* out);

};


/**
 * A binary classifier. TODO Better name? More concrete?
 */
struct cnPredicate {

  void* data;

  //  cnCount inCount;
  //
  //  cnTopology inTopology;
  //
  //  cnType* inType;

  void (*dispose)(cnPredicate* predicate);

  /**
   * Classify the given value (point, bag, ...) as true or false.
   *
   * TODO Error indicated by result other than true or false? Maybe too sneaky.
   */
  cnBool (*evaluate)(cnPredicate* predicate, void* in);

};


struct cnProperty {

  cnType* containerType;

  cnType* type;

  cnString name;

  cnTopology topology;

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
  void (*dispose)(cnProperty* property);

  void (*get)(const cnProperty* property, const void* entity, void* storage);

  void (*put)(const cnProperty* property, void* entity, const void* value);

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

  cnList(cnType) types;

};


struct cnType {

  cnString name;

  cnList(cnProperty) properties;

  cnSchema* schema;

  cnCount size;

};


void cnBagDispose(cnBag* bag);


void cnBagInit(cnBag* bag);


cnEntityFunction* cnEntityFunctionCreateDifference(
  const cnEntityFunction* base
);


cnEntityFunction* cnEntityFunctionCreateDistance(const cnEntityFunction* base);


/**
 * Creates an entity function that just performs a property get.
 */
cnEntityFunction* cnEntityFunctionCreateProperty(const cnProperty* property);


void cnEntityFunctionDrop(cnEntityFunction* function);


/**
 * Disposes of and frees the function if not null.
 */
void cnFunctionDrop(cnFunction* function);


/**
 * Disposes of and frees the predicate if not null.
 */
void cnPredicateDrop(cnPredicate* predicate);


/**
 * The distanceFunction will be dropped with this predicate.
 */
cnPredicate* cnPredicateCreateDistanceThreshold(
  cnFunction* distanceFunction, cnFloat threshold
);


/**
 * Just disposes of the name for now.
 *
 * Doesn't dispose of nor free the type. Types manage properties, not vice
 * versa.
 */
void cnPropertyDispose(cnProperty* property);


/**
 * Provides simple struct field access by offset. The topology is defaulted to
 * Euclidean. Change after the fact for others.
 *
 * On failure, leaves the property in a stable (nulled out) state.
 */
cnBool cnPropertyInitField(
  cnProperty* property, cnType* containerType, cnType* type, char* name,
  cnCount offset, cnCount count
);


/**
 * Disposes all contained types and properties, too.
 */
void cnSchemaDispose(cnSchema* schema);


/**
 * Provides just float (double) type for now.
 *
 * On failure, leaves the schema in a stable (nulled out) state.
 */
cnBool cnSchemaInitDefault(cnSchema* schema);


/**
 * Disposes of all contained properties, too. Leaves name empty, size 0, and
 * schema NULL.
 *
 * Doesn't dispose of the schema. Schemas manage types, not vice versa.
 */
void cnTypeDispose(cnType* type);


/**
 * On failure, leaves the type in a stable (nulled out) state.
 */
cnBool cnTypeInit(cnType* type, char* name, cnCount size);


#endif
