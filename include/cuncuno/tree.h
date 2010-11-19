#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"
#include "distrib.h"

namespace cuncuno {

struct Sample;

/**
 * Provides efficient point-toward-root trees of variable bindings. Allows
 * simple homogeneous storage in nodes of decision trees.
 */
struct Binding {

  /**
   * A dangerous constructor for convenient efficiency in collections.
   */
  Binding();

  /**
   * Creates a binding root referencing the sample but without any actual
   * binding.
   */
  Binding(const Sample& sample);

  /**
   * Creates a binding to an entity with prior bindings as given. The variable
   * in question is unspecified. The binding needs paired with some node in a
   * tree to make sense. We save space to avoid referencing them here.
   */
  Binding(const Binding& previous, const void* entity);

  /**
   * The number of entities bound to vars here.
   */
  Count count() const;

  /**
   * A list of the entities starting from the root towards this level. That is,
   * the current entity is last.
   *
   * TODO Provide a raw pointer buffer version, too?
   */
  const void entities(std::vector<const void*>& buffer) const;

  /**
   * The sample from which these entities were extracted.
   */
  const Sample& sample() const;

private:

  const Binding* previous;

  const void* entityOrSample;

};

/**
 * A generic tree node. It's a tree in the sense that the kids are values, not
 * pointers or references. Makes tear-down easy. And a tree is all we need for
 * now.
 *
 * Subtypes provide data.
 *
 * TODO Should I use pointers to make this into a arbitrary graph node? Would
 * TODO need fancier destructor plans.
 */
struct Node {

  Node(const Node* parent, Count varDepth);

  void binding(Count index, Binding& binding) const;

  Count bindingCount() const;

  /**
   * Pushes a binding as a sample reference and an array of varDepth entity
   * pointers.
   */
  void bindingPush(const Binding& binding);

  const Node* parent;

  std::vector<Node> kids;

  const Count varDepth;

};

/**
 * A node that is neither the root nor a var node. It references existing
 * binding rather than building new ones. For now, these include predicates and
 * leaves.
 */
struct ArrivalNode: Node {
  // TODO

  /**
   * Note that these are pointers to bindings instead of bindings themselves.
   */
  std::vector<Binding*> bindings;

};

/**
 * Determines truth or falsehood on bindings for some abstract criterion.
 *
 * Indicate which types of entities are supported?
 *
 * Note: In SMRF, question nodes have models. Models have PDFs, and PDFs have
 * thresholds. In a sense, a threshold just turns a
 */
template<typename Value>
struct PredicateNode: ArrivalNode {

  /**
   * Returns whether or not the entity matches this predicate. The entity could
   * be composite.
   *
   * Errors, if any, will be thrown.
   */
  bool classify(const void* entity);

  /**
   * This is the same as the one-arg classify, except that error conditions are
   * provided via the error parameter instead of being thrown.
   *
   * TODO Provide an enum for tri-state bools?
   */
  bool classify(const void* entity, bool& error);

  /**
   * Allows extracting values from entities.
   */
  Attribute& attribute;

  /**
   * Determines a probability of some value matching a concept. Extracted
   * attribute values are checked here.
   */
  Pdf& pdf;

  /**
   * The threshold should be a probability that the entity matches the local
   * concept for this question. This could be chosen to maximize some objective.
   *
   * TODO Alternatively normalize PDFs such that the threshold is always 0.5.
   */
  Float threshold;

};

struct VarNode: Node {

  std::vector<Binding> bindings;

};

/**
 * Represents the root of the tree.
 */
struct Tree: Node {

  // Um, TODO.
  Tree(const Type& entityType, const std::vector<Sample>& samples);

  const Type& entityType;

  /**
   * I really don't like this here, actually. I would like to be able to have
   * an abstract tree without samples attached. Is it worth subtyping to get
   * that?
   */
  std::vector<Binding> bindingRoots;

};

}

#endif
