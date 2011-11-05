#ifndef concuno_entity_h
#define concuno_entity_h


#include <string>
#include <vector>

#include "core.h"


namespace concuno {


/**
 * Entities are defined entirely by properties and entity functions.
 */
typedef void* Entity;


struct Bag {

  /**
   * Creates an entity list for managing here.
   */
  Bag();

  /**
   * Uses the given entity list.
   */
  Bag(List<Entity>* entities);

  ~Bag();

  /**
   * Pushes on the given participant to the options for the given depth (or
   * parameter index, so to speak).
   */
  void pushParticipant(Index depth, Entity participant);

  // TODO id? Or are pointer addresses good enough (if stable)?

  /**
   * Entities in the pool. The same entity list might be shared by multiple
   * bag samples. Therefore, it is _not_ disposed of with the bag.
   *
   * TODO Actually, it currently is deleted if not null. Change this?
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


/**
 * TODO Do we really need a topology struct? For now, makes Eclipse happier.
 */
struct Topology {

  enum Type {

    /**
     * Also called standard or real product topology or such like. Just presumed
     * open (to machine limits) R^N space here.
     */
    Euclidean,

    // TODO Use general SpecialOrthogonal and SpecialEuclidean across different
    // TODO dimensionalities?
    //Circle,

    // TODO The following is the power set, actually, but for discrete space
    // TODO allows what we want for discrete properties, I think.
    //Discrete,

  };

};


// Necessary forward declarations.
struct Property;
struct Schema;
struct Type;


/**
 * Maps one or more entities to one or more values. Such functions should
 * always be organized in a context where the entity type is understood or
 * otherwise discernable by the function pointers here.
 */
struct EntityFunction {

  EntityFunction(const char* name, Count inCount, Count outCount);

  virtual ~EntityFunction();

  /**
   * Receives an array of pointers to entities, and provides an array of values
   * whose individual sizes are given by outType.
   */
  virtual void get(Entity* ins, void* outs) = 0;

  /**
   * Pushes this onto functions or deletes this.
   */
  void pushOrDelete(std::vector<EntityFunction*>& functions);

  Count inCount;

  std::string name;

  Count outCount;

  Topology::Type outTopology;

  Type* outType;

};


/**
 * Abstract base type for entity functions working with the results of a base
 * function.
 */
struct ComposedEntityFunction: EntityFunction {

  ComposedEntityFunction(
    EntityFunction& base, const char* name, Count inCount, Count outCount
  );

  EntityFunction& base;

};


struct DifferenceEntityFunction: ComposedEntityFunction {

  /**
   * The handy static form is useful to third parties like distance.
   */
  static void get(EntityFunction& base, Entity* ins, void* outs);

  DifferenceEntityFunction(EntityFunction& base);

  virtual void get(Entity* ins, void* outs);

};


struct DistanceEntityFunction: ComposedEntityFunction {

  DistanceEntityFunction(EntityFunction& base);

  virtual void get(Entity* ins, void* outs);

};


/**
 * An entity function that just performs a property get.
 */
struct PropertyEntityFunction: EntityFunction {

  PropertyEntityFunction(Property& property);

  virtual void get(Entity* ins, void* outs);

  Property& property;

};


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
struct ReframeEntityFunction: ComposedEntityFunction {

  ReframeEntityFunction(EntityFunction& base);

  virtual void get(Entity* ins, void* outs);

};


/**
 * An entity function that determines merely whether all the arguments actually
 * have bindings.
 *
 * These functions should be considered before others, if the aim is for simple
 * and comprehensible trees.
 *
 * TODO Consider error branches off var nodes?
 */
struct ValidityEntityFunction: EntityFunction {

  ValidityEntityFunction(Schema& schema, Count arity);

  virtual void get(Entity* ins, void* outs);

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

  virtual ~Predicate();

  //  Count inCount;
  //
  //  Topology inTopology;
  //
  //  Type* inType;

  virtual Predicate* copy() = 0;

  /**
   * Classify the given value (point, bag, ...) as true or false.
   *
   * TODO Error indicated by result other than true or false? Maybe too sneaky.
   */
  virtual bool evaluate(void* in) = 0;

  /**
   * Writes the predicate in JSON format without surrounding whitespace.
   *
   * TODO Instead provide structured, reflective access (such as via property
   * TODO metadata or hashtables), and have various IO elsewhere?
   */
  virtual void write(FILE* file, String* indent) = 0;

};


struct DistanceThresholdPredicate: Predicate {

  /**
   * The distanceFunction will be deleted with this predicate.
   */
  DistanceThresholdPredicate(Function* distanceFunction, Float threshold);

  virtual ~DistanceThresholdPredicate();

  virtual Predicate* copy();

  /**
   * Classify the given value (point, bag, ...) as true or false.
   *
   * TODO Error indicated by result other than true or false? Maybe too sneaky.
   */
  virtual bool evaluate(void* in);

  /**
   * Writes the predicate in JSON format without surrounding whitespace.
   *
   * TODO Instead provide structured, reflective access (such as via property
   * TODO metadata or hashtables), and have various IO elsewhere?
   */
  virtual void write(FILE* file, String* indent);

  Function* distanceFunction;

  Float threshold;

};


struct Property {

  Property(Type* containerType, Type* type, const char* name, Count count);

  /**
   * Doesn't dispose of nor free the type. Types manage properties, not vice
   * versa.
   */
  virtual ~Property();

  virtual void get(Entity entity, void* storage) = 0;

  virtual void put(Entity entity, void* value) = 0;

  Type* containerType;

  Type* type;

  std::string name;

  Topology::Type topology;

  /**
   * If this is an array/list property, says how many there are.
   *
   * If this varies by entity, then it's not useful for building matrices.
   * Therefore, this is a fixed count. A varied length would need to be
   * provided by copying out a pointer here or something.
   */
  Count count;

};


/**
 * For fields where offsets don't work.
 */
struct FieldProperty: Property {

  FieldProperty(
    Type* containerType, Type* type, const char* name, Count count
  );

  virtual void get(Entity entity, void* storage);

  virtual void put(Entity entity, void* value);

  /**
   * Implement this method to provide field access.
   */
  virtual void* field(Entity entity) = 0;

};


struct OffsetProperty: Property {

  OffsetProperty(
    Type* containerType, Type* type, const char* name, Count offset, Count count
  );

  virtual void get(Entity entity, void* storage);

  virtual void put(Entity entity, void* value);

  /**
   * For struct field access.
   */
  Count offset;

};


/**
 * Provides functions for accessing entity attributes.
 */
struct Schema {

  Schema();

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
  AutoVec<Type*> types;

};


struct Type {

  /**
   * Provides float (double) type by default for now.
   */
  Type(Schema& schema, const char* name, Count size);

  std::string name;

  AutoVec<Property*> properties;

  Schema* schema;

  Count size;

};


/**
 * Disposes of all bags and entities in the given lists, as well as all the
 * lists themselves. If entityLists is non-null, entities in each bag will be
 * ignored. Otherwise, entities in bags will be disposed with the bags.
 */
void cnBagListDispose(
  List<Bag>* bags, List<List<Entity>*>* entityLists
);


/**
 * Disposes of and frees the function if not null.
 */
Function* cnFunctionCopy(Function* function);


/**
 * Disposes of and frees the function if not null.
 */
void cnFunctionDrop(Function* function);


}


#endif
