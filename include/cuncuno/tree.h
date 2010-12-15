#ifndef cuncuno_tree_h
#define cuncuno_tree_h

#include <algorithm>
#include "entity.h"
#include "predicate.h"

namespace cuncuno {

struct Joint;
struct LeafNode;
struct Node;
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
   * The entity at the current level, or 0 if at the sample.
   */
  const void* entity() const;

  /**
   * The sample from which these entities were extracted.
   */
  const Sample& sample() const;

  /**
   * The previous binding in the chain, if any.
   */
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

  // For all nodes, called before the type-specific version.
  virtual void visit(Node& node, void* data) {}

  // For specific types.
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
 * A connection point from a node to a potential child. Used as a placeholder
 * for specifying locations for kids.
 */
struct Joint {

  Node* node;

  std::vector<Node*>::iterator location;

};

/**
 * A generic tree node. It's a tree in the sense that the kids are values, not
 * pointers or references. Makes tear-down easy. And a tree is all we need for
 * now.
 *
 * Subtypes provide data.
 *
 * TODO Should I make this into a arbitrary graph node? Would need fancier
 * TODO destructor plans.
 */
struct Node {

  typedef int Id;

  Node(Id id = 0);

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

  /**
   * Returns this or some descendent with the given id if found, else null.
   */
  Node* findById(Id id);

  /**
   * Inserts a new parent before this. If this already has a parent, the new
   * parent will go in its kids in the place of this node. This node goes into
   * place index in the new parent. Any prior kid at that index will be purged.
   *
   * No new kid spots will be added to the new parent. The spot must already be
   * available.
   *
   * The parent will be given a new id. TODO Allow control over this?
   *
   * TODO Allow returning a replaced kid instead of purging it?
   */
  void insertParent(Node& parent, Count index = 0);

  void leaves(std::vector<LeafNode*>& buffer);

  // TODO 'operator =' to copy kids? (Leaving current parent?)

  /**
   * TODO Go to direct field access?
   */
  Node* parent();

  /**
   * Propagate the bindings at this node down through kids.
   */
  virtual void propagate();

  /**
   * Propagates bindings as pointers, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding*>& binding) = 0;

  /**
   * Propagates bindings stored in place, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding>& binding) = 0;

  /**
   * Propagate the bindings at this node down through the given node. It might
   * or might not be a kid of this.
   *
   * Even for question nodes, this propagates all of its bindings to the given
   * node. This comes in handy when inserting parents.
   * TODO Just unify with parent insertion?
   */
  virtual void propagateTo(Node& node) = 0;

  /**
   * At the parent, puts a null in place of this kid, deletes this node, and
   * puts in joint the spot where the kid used to be.
   */
  void purge(Joint& joint);

  /**
   * Push a kid onto this node, assigning its parent (this) and a new ID (if
   * this has a root).
   */
  void pushKid(Node& kid);

  /**
   * Puts node in the parent in place of this, leaving this detached but not
   * destroyed.
   *
   * TODO Guarantee that this isn't replaced if exceptions are thrown?
   */
  void replaceWith(Node& node);

  /**
   * The highest node up the tree if it's a RootNode else null.
   */
  RootNode* root();

  /**
   * Visits first the current node then the kids in order.
   */
  void traverse(NodeVisitor& visitor, void* data = 0);

  Id id;

  /**
   * Nodes are not guaranteed to be of the same size, although maybe I could do
   * that with unions (and still keep nodes generic through templates). That's
   * why the use of pointers here. Could have a pools of nodes of different
   * types for efficiency, but I don't expect large numbers of kids anyway, so
   * I doubt it's a big deal.
   */
  std::vector<Node*> kids;

  Node* $parent;

};

/**
 * A node that is neither the root nor a var node. It references existing
 * binding rather than building new ones. For now, these include predicates and
 * leaves.
 */
struct ArrivalNode: Node {

  ArrivalNode();

  ArrivalNode(const ArrivalNode& other);

  ~ArrivalNode();

  /**
   * Propagate the bindings at this node down through the given node. It might
   * or might not be a kid of this.
   */
  virtual void propagateTo(Node& node);

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
struct StorageNode: Node {

  StorageNode(Id id = 0);

  StorageNode(const StorageNode& other);

  ~StorageNode();

  /**
   * Propagate the bindings at this node down through the given node. It might
   * or might not be a kid of this.
   */
  virtual void propagateTo(Node& node);

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
   * Propagates bindings as pointers, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding*>& binding);

  /**
   * Propagates bindings stored in place, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding>& binding);

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

  /**
   * Propagates bindings as pointers, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding*>& binding);

  /**
   * Propagates bindings stored in place, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding>& binding);

  FunctionPredicate* predicate;

  Node* $true;

  Node* $false;

  Node* error;

};

/**
 * Represents the root of the tree.
 */
struct RootNode: StorageNode {

  /**
   * Copies the tree structure but not the bindings.
   */
  RootNode(const RootNode& other);

  /**
   * For a new tree with the given entity type.
   */
  RootNode(const Type& entityType, Node* kid = new LeafNode);

  virtual void accept(NodeVisitor& visitor, void* data);

  void bindingsPush(const std::vector<Sample>& samples);

  virtual Node* copy();

  Id generateId();

  /**
   * Propagates bindings as pointers, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding*>& binding);

  /**
   * Propagates bindings stored in place, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding>& binding);

  /**
   * Propagate bindings starting from the given samples, calling the given
   * visitor for each node, including this.
   *
   * The samples are also pushed as new bindings to this node.
   */
  void propagate(const std::vector<Sample>& samples);

  /**
   * Although just one type is supported, technically this could dispatch on
   * subinformation. The optimization toward a single type presumes that most
   * entities for a learning problem will have similar attributes.
   */
  const Type& entityType;

  Node* kid;

private:

  Id nextId;

};

struct VariableNode: StorageNode {

  /**
   * Creates a VariableNode with the given kid, defaulting to a new leaf.
   */
  VariableNode(Node* kid = new LeafNode);

  VariableNode(const VariableNode& other);

  virtual void accept(NodeVisitor& visitor, void* data);

  virtual Node* copy();

  /**
   * Propagates bindings as pointers, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding*>& binding);

  /**
   * Propagates bindings stored in place, replacing any existing bindings.
   */
  virtual void propagate(std::vector<Binding>& binding);

  Node* kid;

};

}

#endif
