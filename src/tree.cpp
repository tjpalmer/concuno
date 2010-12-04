#include "learner.h"
#include "tree.h"

#include <iostream>

using namespace std;

namespace cuncuno {


template<typename Value>
void pushPointers(
  std::vector<Value>& values, std::vector<Value*>& pointers
) {
  for (
    typename std::vector<Value>::iterator v(values.begin());
    v != values.end();
    v++
  ) {
    pointers.push_back(&*v);
  }
}


/// ArrivalNode.

ArrivalNode::ArrivalNode(): arrival(new BindingArrival) {}

ArrivalNode::ArrivalNode(const ArrivalNode& other):
  KidNode(other), arrival(other.arrival) {}

ArrivalNode::~ArrivalNode() {
  arrival->release();
}


/// Binding.

Binding::Binding() {}

Binding::Binding(const Binding& $previous, const void* entity):
  previous(&$previous), entityOrSample(entity) {}

Binding::Binding(const Sample& sample):
  previous(0), entityOrSample(&sample) {}

Count Binding::count() const {
  Count count(0);
  for (
    const Binding* binding(this); binding->previous; binding = binding->previous
  ) {
    count++;
  }
  return count;
}

const void Binding::entities(std::vector<const void*>& buffer) const {
  Count count = this->count();
  buffer.clear();
  buffer.resize(count);
  for (
    const Binding* binding(this); binding->previous; binding = binding->previous
  ) {
    // Fill in reverse order, so subtract count as we go. Also, no need to check
    // bounds. We already made sure we had the right size.
    buffer[--count] = binding->entityOrSample;
  }
}

const Sample& Binding::sample() const {
  const Binding* binding(this);
  while (binding->previous) {
    binding = binding->previous;
  }
  return *reinterpret_cast<const Sample*>(entityOrSample);
}


/// KidNode.

KidNode::KidNode(): $parent(0) {}

KidNode::KidNode(const KidNode& other): Node(other), $parent(0) {}

Node* KidNode::parent() {
  return $parent;
}


/// LeafNode.

LeafNode::LeafNode() {}

LeafNode::LeafNode(const LeafNode& other):
  ArrivalNode(other), probability(other.probability) {}

void LeafNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

Node* LeafNode::copy() {
  return new LeafNode(*this);
}


/// Node.

Node::Node(Id $id): id($id) {}

Node::Node(const Node& other): id(other.id) {
  for (
    vector<KidNode*>::const_iterator k(other.kids.begin());
    k != other.kids.end();
    k++
  ) {
    KidNode* kid = dynamic_cast<KidNode*>((*k)->copy());
    kid->$parent = this;
    // Don't use pushKid, because we want to keep the ID in this case.
    kids.push_back(kid);
  }
}

Node::~Node() {
  while (!kids.empty()) {
    delete kids.back();
    kids.pop_back();
  }
}

Node* Node::findById(Node::Id id) {
  if (this->id == id) {
    return this;
  }
  for (vector<KidNode*>::iterator k = kids.begin(); k != kids.end(); k++) {
    Node* node = (*k)->findById(id);
    if (node) {
      return node;
    }
  }
  // No match found.
  return 0;
}

void Node::leaves(std::vector<LeafNode*>& buffer) {
  struct LeafVisitor: NodeVisitor {
    LeafVisitor(std::vector<LeafNode*>& $buffer): buffer($buffer) {}
    virtual void visit(LeafNode& node, void* data) {
      buffer.push_back(&node);
    }
    std::vector<LeafNode*>& buffer;
  };
  LeafVisitor visitor(buffer);
  traverse(visitor);
}

Node* Node::parent() {
  return 0;
}

void Node::propagate(
  BindingsNodeVisitor& visitor, std::vector<Binding>& bindings
) {
  std::vector<Binding*> pointers;
  pushPointers(bindings, pointers);
  propagate(visitor, pointers);
}

void Node::propagate(
  BindingsNodeVisitor& visitor, std::vector<Binding*>& bindings
) {
  struct PropagateVisitor: BindingsNodeVisitor, Worker {
    PropagateVisitor(BindingsNodeVisitor& $visitor):
      Worker("PropagateVisitor"), visitor($visitor) {}
    virtual void visit(LeafNode& node, std::vector<Binding*>& bindings) {
      // Store the bindings in the node.
      // TODO Find the generic algorithm for push all.
      for (
        std::vector<Binding*>::iterator b = bindings.begin();
        b != bindings.end();
        b++
      ) {
        node.arrival->bindings.push_back(*b);
      }
      // Now visit our visitor.
      visitor.visit(node, node.arrival->bindings);
    }
    virtual void visit(PredicateNode& node, std::vector<Binding*>& bindings) {
      visitor.visit(node, bindings);
      // TODO Split on predicate.
    }
    virtual void visit(VariableNode& node, std::vector<Binding*>& bindings) {
      visitor.visit(node, bindings);
      // TODO Bind entities to variables.
    }
    virtual void visit(RootNode& node, std::vector<Binding*>& bindings) {
      visitor.visit(node, bindings);
      for (
        std::vector<KidNode*>::iterator k = node.kids.begin();
        k != node.kids.end();
        k++
      ) {
        (*k)->accept(*this, &bindings);
      }
    }
    BindingsNodeVisitor& visitor;
  };
  PropagateVisitor propagator(visitor);
  accept(propagator, &bindings);
}

void Node::pushKid(KidNode& kid) {
  RootNode* root = this->root();
  if (root) {
    kid.id = root->generateId();
  }
  kid.$parent = this;
  kids.push_back(&kid);
}

RootNode* Node::root() {
  Node* highest = this;
  while (parent()) {
    highest = parent();
  }
  return dynamic_cast<RootNode*>(highest);
}

void Node::traverse(NodeVisitor& visitor, void* data) {
  accept(visitor, data);
  for (std::vector<KidNode*>::iterator k = kids.begin(); k != kids.end(); k++) {
    (*k)->traverse(visitor, data);
  }
}


/// NodeStorage.

NodeStorage::NodeStorage(): storage(new BindingStorage) {}

NodeStorage::NodeStorage(const NodeStorage& other): storage(other.storage) {
  // We are another client for this storage.
  storage->acquire();
}

NodeStorage::~NodeStorage() {
  storage->release();
}


/// PredicateNode.

PredicateNode::PredicateNode() {}

PredicateNode::PredicateNode(const PredicateNode& other):
  // TODO Copy the predicate to make sure the tree really is copied???
  // TODO Could that be slow? Make it instead const and Shared?
  ArrivalNode(other), predicate(other.predicate) {}

void PredicateNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

Node* PredicateNode::copy() {
  return new PredicateNode(*this);
}


/// RootNode.

RootNode::RootNode(const RootNode& other):
  Node(other),
  NodeStorage(other),
  entityType(other.entityType),
  nextId(other.nextId) {}

RootNode::RootNode(const Type& $entityType):
  Node(1), entityType($entityType), nextId(id + 1) {}

void RootNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

void RootNode::basicTree() {
  // TODO Check for already existing kids and if so then bail out (with or
  // TODO without error?).
  // TODO Or make this a static method?
  pushKid(*new LeafNode);
}

void RootNode::bindingsPush(const std::vector<Sample>& samples) {
  storage->bindings.reserve(samples.size());
  for (Count s = 0; s < samples.size(); s++) {
    storage->bindings.push_back(Binding(samples[s]));
  }
}

Node* RootNode::copy() {
  return new RootNode(*this);
}

Node::Id RootNode::generateId() {
  return nextId++;
}

void RootNode::propagate(
  BindingsNodeVisitor& visitor, const std::vector<Sample>& samples
) {
  // Put the samples into bindings.
  for (
    std::vector<Sample>::const_iterator s = samples.begin();
    s != samples.end();
    s++
  ) {
    storage->bindings.push_back(Binding(*s));
  }
  // Now propagate the bindings.
  Node::propagate(visitor, storage->bindings);
}


/// VariableNode.

VariableNode::VariableNode() {}

VariableNode::VariableNode(const VariableNode& other):
  KidNode(other), NodeStorage(other) {}

void VariableNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

Node* VariableNode::copy() {
  return new VariableNode(*this);
}


}
