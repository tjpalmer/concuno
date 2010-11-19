#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"
#include "distrib.h"

namespace cuncuno {

struct Sample;

struct Binding {

  const Sample* sample;

  std::vector<const void*> entities;

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

private:

  /**
   * Size varDepth * binding count.
   *
   * This and samples are split out to make storage more efficient, since these
   * arrays can get rather large. The down side is more potential copying. I
   * also looked at hacking a variable length struct and then making grids of
   * those, but I felt like that was more effort and risky.
   *
   * Anyway, this does give fewer allocated memory blocks overall, and that
   * seems likely to be a win.
   */
  std::vector<const void*> entities;

  /**
   * Size binding count.
   */
  std::vector<const Sample*> samples;

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
  // TODO Just a placeholder? Does anything special go here? Info to say it's a
  // TODO var? (Type pointer?)
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
  const std::vector<Sample>& samples;

};

}

#endif
