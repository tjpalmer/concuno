#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"
#include "predicate.h"

namespace cuncuno {

struct LeafNode;
struct NodeVisitor;
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

  // TODO Copy constructor and 'operator =' to copy kids?

  /**
   * Deletes kids.
   */
  virtual ~Node();

  virtual void accept(NodeVisitor& visitor, void* data) = 0;

  void leaves(std::vector<LeafNode*>& buffer);

  virtual Node* parent();

  /**
   * Nodes are not guaranteed to be of the same size, although maybe I could do
   * that with unions (and still keep nodes generic through templates). That's
   * why the use of pointers here. Could have a pools of nodes of different
   * types for efficiency, but I don't expect large numbers of kids anyway, so
   * I doubt it's a big deal.
   */
  std::vector<Node*> kids;

};

struct KidNode {

  KidNode();

  virtual Node* parent();

  Node* $parent;

};

/**
 * A node that is neither the root nor a var node. It references existing
 * binding rather than building new ones. For now, these include predicates and
 * leaves.
 */
struct ArrivalNode: KidNode {

  /**
   * Note that these are pointers to bindings instead of bindings themselves.
   */
  std::vector<Binding*> bindings;

};

struct LeafNode: ArrivalNode {

  virtual void accept(NodeVisitor& visitor, void* data);

  Float probability;

};

/**
 * Applies an attribute predicate to bound entities.
 */
struct PredicateNode: ArrivalNode {

  virtual void accept(NodeVisitor& visitor, void* data);

  AttributePredicate* predicate;

};

/**
 * Represents the root of the tree.
 */
struct RootNode: Node {

  RootNode(const Type& entityType);

  virtual void accept(NodeVisitor& visitor, void* data);

  void bindingsPush(const std::vector<Sample>& samples);

  /**
   * Although just one type is supported, technically this could dispatch on
   * subinformation. The optimization toward a single type presumes that most
   * entities for a learning problem will have similar attributes.
   */
  const Type& entityType;

  /**
   * I really don't like this here, actually. I would like to be able to have
   * an abstract tree without samples attached. Is it worth subtyping to get
   * that?
   */
  std::vector<Binding> bindings;

};

struct VariableNode: KidNode {

  virtual void accept(NodeVisitor& visitor, void* data);

  std::vector<Binding> bindings;

};

struct NodeVisitor {
  virtual void visit(LeafNode& node, void* data);
  virtual void visit(PredicateNode& node, void* data);
  virtual void visit(VariableNode& node, void* data);
  virtual void visit(RootNode& node, void* data);
};

}

#endif
