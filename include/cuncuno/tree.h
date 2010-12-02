#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include "entity.h"
#include "predicate.h"

namespace cuncuno {

struct LeafNode;
struct PredicateNode;
struct RootNode;
struct Sample;
struct VariableNode;

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

struct BindingArrival: Shared {
  std::vector<Binding*> bindings;
};

struct BindingStorage: Shared {
  std::vector<Binding> bindings;
};

/**
 * Simple void* node visitor for heterogeneous use.
 */
struct NodeVisitor {
  // Do nothing by default for all of these. Subtypes override as they see fit.
  virtual void visit(LeafNode& node, void* data) {}
  virtual void visit(PredicateNode& node, void* data) {}
  virtual void visit(VariableNode& node, void* data) {}
  virtual void visit(RootNode& node, void* data) {}
};

/**
 * Specialization when you want to clearly define what homogeneous data will be
 * received.
 */
template<typename Data>
struct NodeVisitorOf: NodeVisitor {

  // Do nothing by default for all of these. Subtypes override as they see fit.
  virtual void visit(LeafNode& node, Data& data) {}
  virtual void visit(PredicateNode& node, Data& data) {}
  virtual void visit(VariableNode& node, Data& data) {}
  virtual void visit(RootNode& node, Data& data) {}

  // Just casts and calls. No need to override these further.
  virtual void visit(LeafNode& node, void* data) {
    visit(node, *reinterpret_cast<Data*>(data));
  }
  virtual void visit(PredicateNode& node, void* data)  {
    visit(node, *reinterpret_cast<Data*>(data));
  }
  virtual void visit(VariableNode& node, void* data)  {
    visit(node, *reinterpret_cast<Data*>(data));
  }
  virtual void visit(RootNode& node, void* data)  {
    visit(node, *reinterpret_cast<Data*>(data));
  }

};

typedef NodeVisitorOf<std::vector<Binding*> > BindingsNodeVisitor;

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

  Node();

  /**
   * Deep copy of tree but with same binding instances and null parent.
   */
  Node(const Node& other);

  /**
   * Deletes kids.
   */
  virtual ~Node();

  virtual void accept(NodeVisitor& visitor, void* data = 0) = 0;

  /**
   * Deep copy of tree but with same binding instances and null parent.
   */
  virtual Node* copy() = 0;

  void leaves(std::vector<LeafNode*>& buffer);

  // TODO 'operator =' to copy kids?

  virtual Node* parent();

  /**
   * Propagates bindings with direct storage through the tree, calling the
   * given visitor for each node, including this.
   */
  void propagate(BindingsNodeVisitor& visitor, std::vector<Binding>& bindings);

  /**
   * Propagates bindings given as pointers through the tree, calling the given
   * visitor for each node, including this.
   */
  void propagate(BindingsNodeVisitor& visitor, std::vector<Binding*>& bindings);

  /**
   * Visits first the current node then the kids in order.
   */
  void traverse(NodeVisitor& visitor, void* data = 0);

  /**
   * Nodes are not guaranteed to be of the same size, although maybe I could do
   * that with unions (and still keep nodes generic through templates). That's
   * why the use of pointers here. Could have a pools of nodes of different
   * types for efficiency, but I don't expect large numbers of kids anyway, so
   * I doubt it's a big deal.
   */
  std::vector<Node*> kids;

};

struct KidNode: virtual Node {

  KidNode();

  KidNode(const KidNode& other);

  virtual Node* parent();

  Node* $parent;

};

/**
 * A node that is neither the root nor a var node. It references existing
 * binding rather than building new ones. For now, these include predicates and
 * leaves.
 */
struct ArrivalNode: KidNode {

  ArrivalNode();

  ArrivalNode(const ArrivalNode& other);

  ~ArrivalNode();

  /**
   * I really don't like this here, actually. I would like to be able to have
   * an abstract tree without samples attached. Is it worth subtyping to get
   * that?
   */
  BindingArrival* arrival;

};

/**
 * Storage of bindings for nodes. Meant to be inherited. Provides some
 * convenience for construction and destruction.
 */
struct NodeStorage {

  NodeStorage();

  NodeStorage(const NodeStorage& other);

  ~NodeStorage();

  /**
   * I really don't like this here, actually. I would like to be able to have
   * an abstract tree without samples attached. Is it worth subtyping to get
   * that?
   */
  BindingStorage* storage;

};

struct LeafNode: ArrivalNode {

  LeafNode();

  LeafNode(const LeafNode& other);

  virtual void accept(NodeVisitor& visitor, void* data);

  virtual Node* copy();

  /**
   * In SMRF, the probability that a binding is an example of the target concept
   * given that it reached this leaf. In some cases, though, I might use this to
   * mean the probability that a bag is true given that any of its bindings
   * reach this point. That might just be during intermediate computation,
   * however.
   *
   * TODO Separate the two probabilities? Make node extensions for custom data?
   */
  Float probability;

};

/**
 * Applies an attribute predicate to bound entities.
 */
struct PredicateNode: ArrivalNode {

  PredicateNode();

  PredicateNode(const PredicateNode& other);

  virtual void accept(NodeVisitor& visitor, void* data);

  virtual Node* copy();

  AttributePredicate* predicate;

};

/**
 * Represents the root of the tree.
 */
struct RootNode: Node, NodeStorage {

  RootNode(const RootNode& other);

  RootNode(const Type& entityType);

  virtual void accept(NodeVisitor& visitor, void* data);

  /**
   * Builds a tree with just one leaf out of the root.
   */
  void basicTree();

  void bindingsPush(const std::vector<Sample>& samples);

  virtual Node* copy();

  /**
   * Propagate bindings starting from the given samples, calling the given
   * visitor for each node, including this.
   *
   * The samples are also pushed as new bindings to this node.
   */
  void propagate(
    BindingsNodeVisitor& visitor, const std::vector<Sample>& samples
  );

  /**
   * Although just one type is supported, technically this could dispatch on
   * subinformation. The optimization toward a single type presumes that most
   * entities for a learning problem will have similar attributes.
   */
  const Type& entityType;

};

struct VariableNode: KidNode, NodeStorage {

  VariableNode();

  VariableNode(const VariableNode& other);

  virtual void accept(NodeVisitor& visitor, void* data);

  virtual Node* copy();

};

}

#endif
