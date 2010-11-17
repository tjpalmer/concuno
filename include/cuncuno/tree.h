#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"
#include "distrib.h"

namespace cuncuno {

struct VarNode;

/**
 * Binding of entity to variable.
 */
struct BindingPair {

  // TODO Can references work with virtuals?
  Entity* entity;

  VarNode& var;

};

/**
 * A tuple of binding pairs. It could perhaps be a map, but we don't expect
 * large numbers of vars at the present, and do we have nice keys for the vars?
 */
struct Binding {
  std::vector<BindingPair> pairs;
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

  std::vector<Node> kids;

  // TODO Pointer to parent?

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
struct PredicateNode: Node {

  /**
   * Returns whether or not the entity matches this predicate. The entity could
   * be composite.
   *
   * Errors, if any, will be thrown.
   */
  bool classify(const Entity* entity);

  /**
   * This is the same as the one-arg classify, except that error conditions are
   * provided via the error parameter instead of being thrown.
   *
   * TODO Provide an enum for tri-state bools?
   */
  bool classify(const Entity* entity, bool& error);

  /**
   * Allows extracting values from entities.
   */
  Attribute<Value>* attribute;

  /**
   * Determines a probability of some value matching a concept. Extracted
   * attribute values are checked here.
   */
  Pdf<Value>* pdf;

  /**
   * The threshold should be a probability that the entity matches the local
   * concept for this question. This could be chosen to maximize some objective.
   *
   * TODO Alternatively normalize PDFs such that the threshold is always 0.5.
   */
  Float threshold;

};

struct FloatPredicateNode: Node {
  // TODO Model which has mapping and pdf which has threshold.
  // TODO In the abstract, attribute is also a mapping function.
  FloatAttribute* attribute;
  // TODO Metric<Value>* metric; // TODO Or tie this to attribute???
  // TODO Float threshold;
};

struct VarNode: Node {
  // TODO Just a placeholder? Does anything special go here? Info to say it's a
  // TODO var? (Type pointer?)
};

struct Tree {
  // Um, TODO.
};

}

#endif
