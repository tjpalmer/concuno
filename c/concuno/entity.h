#ifndef concuno_entity_h
#define concuno_entity_h


#include <vector>

#include "core.h"


namespace concuno {


/**
 * Entities are defined entirely by properties and entity functions.
 */
typedef void* Entity;


struct Bag {

  Bag();

  Bag(cnList(Entity)* entities);

  ~Bag();

  void dispose();

  void init(cnList(Entity)* entities = 0);

  /**
   * Pushes on the given participant to the options for the given depth (or
   * parameter index, so to speak).
   */
  void pushParticipant(Index depth, Entity participant);

  // TODO id? Or are pointer addresses good enough (if stable)?

  /**
   * Entities in the pool. The same entity list might be shared by multiple
   * bag samples. Therefore, it is _not_ disposed of with the bag.
   */
  List<Entity>* entities;

  /**
   * Positive or negative bag.
   */
  bool label;

  /**
   * Entities known to participate in the labeling. These should be bound to
   * vars first, in the order given here. Each index in the outer list
   * corresponds to the same indexed var down a tree branch.
   *
   * For learning, do not provide any functions used in originally identifying
   * the participants, or you might only learn what you already know.
   *
   * These participant lists are considered to be owned by the bag but not the
   * actual entities themselves.
   *
   * TODO This doesn't allow for intermediate null lists (say constraining var
   * TODO 0 and var 2 but not 1), unless we say that empty lists mean
   * TODO unconstrained rather than no options. Would we ever want to say no
   * TODO options? If so, we need to allow null to distinguish from empty,
   * TODO which means storing pointers or maybe a struct with a separate bool.
   */
  List<List<Entity> > participantOptions;

};


typedef enum {

  /**
   * Also called standard or real product topology or such like. Just presumed
   * open (to machine limits) R^N space here.
   */
  TopologyEuclidean,

  // TODO Use general SpecialOrthogonal and SpecialEuclidean across different
  // TODO dimensionalities?
  //TopologyCircle,

  // TODO The following is the power set, actually, but for discrete space
  // TODO allows what we want for discrete properties, I think.
  //TopologyDiscrete,

} Topology;


// Necessary forward declaration.
struct Type;


/**
 * Maps one or more entities to one or more values. Such functions should
 * always be organized in a context where the entity type is understood or
 * otherwise discernable by the function pointers here.
 */
struct EntityFunction {

  virtual ~EntityFunction();

  void* data;

  Count inCount;

  String name;

  Count outCount;

  Topology outTopology;

  Type* outType;

  /**
   * If not null, call this before finishing generic disposal.
   */
  void (*dispose)(const EntityFunction* function);

  /**
   * Receives an array of pointers to entities, and provides an array of values
   * whose individual sizes are given by outType.
   */
  void (*get)(EntityFunction* function, Entity* ins, void* outs);

};


/**
 * Just a generic function with private data. The meaning depends on context.
 *
 * TODO Unify function and predicate! (Maybe even fold in entity function?)
 */
struct Function {

  void* info;

  // TODO Name or types or anything?

  Function* (*copy)(Function* predicate);

  void (*dispose)(Function* function);

  /**
   * Return value indicates status.
   */
  bool (*evaluate)(Function* function, void* in, void* out);

  /**
   * Writes the predicate in JSON format without surrounding whitespace.
   *
   * TODO Instead provide structured, reflective access (such as via property
   * TODO metadata or hashtables), and have various IO elsewhere?
   */
  bool (*write)(Function* function, FILE* file, String* indent);

};


/**
 * A binary classifier. TODO Better name? More concrete?
 */
struct Predicate {

  void* info;

  //  Count inCount;
  //
  //  Topology inTopology;
  //
  //  Type* inType;

  Predicate* (*copy)(Predicate* predicate);

  void (*dispose)(Predicate* predicate);

  /**
   * Classify the given value (point, bag, ...) as true or false.
   *
   * TODO Error indicated by result other than true or false? Maybe too sneaky.
   */
  bool (*evaluate)(Predicate* predicate, void* in);

  /**
   * Writes the predicate in JSON format without surrounding whitespace.
   *
   * TODO Instead provide structured, reflective access (such as via property
   * TODO metadata or hashtables), and have various IO elsewhere?
   */
  bool (*write)(Predicate* predicate, FILE* file, String* indent);

};


struct PredicateThresholdInfo {

  Function* distanceFunction;

  Float threshold;

};


struct Property {

  Type* containerType;

  Type* type;

  String name;

  Topology topology;

  /**
   * If this is an array/list property, says how many there are.
   *
   * If this varies by entity, then it's not useful for building matrices.
   * Therefore, this is a fixed count. A varied length would need to be
   * provided by copying out a pointer here or something.
   */
  Count count;

  union {

    /**
     * For more abstract properties, as needed.
     */
    void* data;

    /**
     * For the common case of struct field access.
     */
    Count offset;

  };

  /**
   * If not null, call this before finishing generic disposal.
   */
  void (*dispose)(Property* property);

  void (*get)(Property* property, Entity entity, void* storage);

  void (*put)(Property* property, Entity entity, void* value);

};


/**
 * Provides functions for accessing entity attributes.
 */
struct Schema {

  /**
   * Corresponds to Float (double).
   *
   * Could be NULL if undefined for this schema.
   */
  Type* floatType;

  /**
   * TODO Should make this a list of pointers to types so that they are stable
   * TODO when expanding the list.
   * TODO
   * TODO Make more things typedef'd to pointers all around? More opaque, too?
   */
  List<Type*> types;

};


struct Type {

  String name;

  /**
   * TODO Store pointers instead of expanded, to keep later pointers here
   * TODO stable?
   */
  List<Property> properties;

  Schema* schema;

  Count size;

};


/**
 * Disposes of all bags and entities in the given lists, as well as all the
 * lists themselves. If entityLists is non-null, entities in each bag will be
 * ignored. Otherwise, entities in bags will be disposed with the bags.
 */
void cnBagListDispose(
  cnList(Bag)* bags, cnList(cnList(Entity)*)* entityLists
);


EntityFunction* cnEntityFunctionCreateDifference(EntityFunction* base);


EntityFunction* cnEntityFunctionCreateDistance(EntityFunction* base);


/**
 * Creates an entity function that just performs a property get.
 */
EntityFunction* cnEntityFunctionCreateProperty(Property* property);


/**
 * A ternary function needing one representing an origin, another representing
 * one unit down the x (first) axis, and another who's coordinate in the new
 * frame is the output value.
 *
 * Beyond two dimensions, the solution is underconstrained. For the moment, no
 * promises are made as to what rotation is applied.
 *
 * TODO Allow greater arity for extra constraints in higher dimensions? High
 * TODO arity is trouble.
 */
EntityFunction* cnEntityFunctionCreateReframe(EntityFunction* base);


/**
 * Creates an entity function that determines merely whether all the arguments
 * actually have bindings.
 *
 * These functions should be considered before others, if the aim is for simple
 * and comprehensible trees.
 *
 * TODO Consider error branches off var nodes?
 */
EntityFunction* cnEntityFunctionCreateValid(Schema* schema, Count arity);


void cnEntityFunctionDrop(EntityFunction* function);


/**
 * Just a basic create for if you want to fill in your own and start out clean.
 */
EntityFunction* cnEntityFunctionCreate(
  const char* name, Count inCount, Count outCount
);


/**
 * Disposes of and frees the function if not null.
 */
Function* cnFunctionCopy(Function* function);


/**
 * Disposes of and frees the function if not null.
 */
void cnFunctionDrop(Function* function);


/**
 * Copies the predicate.
 */
Predicate* cnPredicateCopy(Predicate* predicate);


/**
 * Disposes of and frees the predicate if not null.
 */
void cnPredicateDrop(Predicate* predicate);


/**
 * The distanceFunction will be dropped with this predicate.
 */
Predicate* cnPredicateCreateDistanceThreshold(
  Function* distanceFunction, Float threshold
);


/**
 * Uses the predicate's write function to write itself to the file as a JSON
 * object.
 */
bool cnPredicateWrite(Predicate* predicate, FILE* file, String* indent);


/**
 * Just disposes of the name for now.
 *
 * Doesn't dispose of nor free the type. Types manage properties, not vice
 * versa.
 */
void cnPropertyDispose(Property* property);


/**
 * Provides simple struct field access by offset. The topology is defaulted to
 * Euclidean. Change after the fact for others.
 *
 * On failure, leaves the property in a stable (nulled out) state.
 */
bool cnPropertyInitField(
  Property* property, Type* containerType, Type* type, const char* name,
  Count offset, Count count
);


EntityFunction* cnPushDifferenceFunction(
  cnList(EntityFunction*)* functions, EntityFunction* base
);


EntityFunction* cnPushDistanceFunction(
  cnList(EntityFunction*)* functions, EntityFunction* base
);


EntityFunction* cnPushPropertyFunction(
  cnList(EntityFunction*)* functions, Property* property
);


EntityFunction* cnPushValidFunction(
  cnList(EntityFunction*)* functions, Schema* schema, Count arity
);


/**
 * Disposes all contained types and properties, too.
 */
void cnSchemaDispose(Schema* schema);


/**
 * Inits just an empty schema.
 */
void cnSchemaInit(Schema* schema);


/**
 * Provides just float (double) type for now.
 *
 * On failure, leaves the schema in a stable (nulled out) state.
 */
bool cnSchemaInitDefault(Schema* schema);


/**
 * Provides just float (double) type for now.
 *
 * On failure, leaves the schema in the original state.
 *
 * TODO Actually keep this here, or just stick to init default above?
 */
bool cnSchemaDefineStandardTypes(Schema* schema);


/**
 * On failure, returns null.
 */
Type* cnTypeCreate(const char* name, Count size);


/**
 * Disposes of all contained properties, and so on, and frees the type.
 *
 * Doesn't dispose of the schema. Schemas manage types, not vice versa.
 */
void cnTypeDrop(Type* type);


}


#endif
