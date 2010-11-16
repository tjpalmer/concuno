#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"

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

struct QuestionNode: Node {
  // TODO Model which has mapping and pdf which has threshold.
  // TODO In the abstract, attribute is also a mapping function.
  // TODO Attribute<Value>* attribute;
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
