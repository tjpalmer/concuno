#ifndef concuno_entity_h
#define concuno_entity_h


#include "core.h"


/**
 * Entities are defined entirely by properties and entity functions.
 */
typedef void* cnEntity;


typedef struct cnBag {

  // TODO id? Or are pointer addresses good enough (if stable)?

  /**
   * Entities in the pool.
   */
  cnList(cnEntity) entities;

  /**
   * Positive or negative bag.
   */
  cnBool label;

  /**
   * Entities known to participate in the labeling. These should be bound to
   * vars first, in the order given here.
   *
   * For learning, do not provide any functions used in originally identifying
   * the participants, or you might only learn what you already know.
   */
  cnList(cnEntity) participants;

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
  void (*get)(cnEntityFunction* function, cnEntity* ins, void* outs);

};


/**
 * Just a generic function with private data. The meaning depends on context.
 */
struct cnFunction {

  void* data;

  // TODO Name or types or anything?

  cnFunction* (*copy)(cnFunction* predicate);

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

  cnPredicate* (*copy)(cnPredicate* predicate);

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

  void (*get)(const cnProperty* property, cnEntity entity, void* storage);

  void (*put)(const cnProperty* property, cnEntity entity, void* value);

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

  /**
   * TODO Should make this a list of pointers to types so that they are stable
   * TODO when expanding the list.
   * TODO
   * TODO Make more things typedef'd to pointers all around? More opaque, too?
   */
  cnList(cnType) types;

};


struct cnType {

  cnString name;

  /**
   * TODO Store pointers instead of expanded, to keep later pointers here
   * TODO stable?
   */
  cnList(cnProperty) properties;

  cnSchema* schema;

  cnCount size;

};


void cnBagDispose(cnBag* bag);


void cnBagInit(cnBag* bag);


cnEntityFunction* cnEntityFunctionCreateDifference(cnEntityFunction* base);


cnEntityFunction* cnEntityFunctionCreateDistance(cnEntityFunction* base);


/**
 * Creates an entity function that just performs a property get.
 */
cnEntityFunction* cnEntityFunctionCreateProperty(cnProperty* property);


void cnEntityFunctionDrop(cnEntityFunction* function);


/**
 * Disposes of and frees the function if not null.
 */
cnFunction* cnFunctionCopy(cnFunction* function);


/**
 * Disposes of and frees the function if not null.
 */
void cnFunctionDrop(cnFunction* function);


/**
 * Copies the predicate.
 */
cnPredicate* cnPredicateCopy(cnPredicate* predicate);


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


cnEntityFunction* cnPushDifferenceFunction(
  cnList(cnEntityFunction*)* functions, cnEntityFunction* base
);


cnEntityFunction* cnPushDistanceFunction(
  cnList(cnEntityFunction*)* functions, cnEntityFunction* base
);


cnEntityFunction* cnPushPropertyFunction(
  cnList(cnEntityFunction*)* functions, cnProperty* property
);


/**
 * Disposes all contained types and properties, too.
 */
void cnSchemaDispose(cnSchema* schema);


/**
 * Inits just an empty schema.
 */
void cnSchemaInit(cnSchema* schema);


/**
 * Provides just float (double) type for now.
 *
 * On failure, leaves the schema in a stable (nulled out) state.
 */
cnBool cnSchemaInitDefault(cnSchema* schema);


/**
 * Provides just float (double) type for now.
 *
 * On failure, leaves the schema in the original state.
 *
 * TODO Actually keep this here, or just stick to init default above?
 */
cnBool cnSchemaDefineStandardTypes(cnSchema* schema);


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
