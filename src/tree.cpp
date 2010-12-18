#include <algorithm>
#include "learner.h"
#include "tree.h"

#include <iostream>
#include <typeinfo>

using namespace std;

namespace cuncuno {


void clearBindings(ArrivalNode& node, Count reserved);
void clearBindings(StorageNode& node, Count reserved);

void receive(LeafNode& node, Binding& binding);
void receive(PredicateNode& node, Binding& binding);
void receive(RootNode& node, Binding& binding);
void receive(VariableNode& node, Binding& binding);

/**
 * Propagates bindings with direct storage through the tree.
 */
template<typename SomeNode>
void receive(SomeNode& node, std::vector<Binding>& bindings) {
  clearBindings(node, bindings.size());
  for (Count b = 0; b < bindings.size(); b++) {
    receive(node, bindings[b]);
  }
}

/**
 * Propagates bindings given as pointers through the tree.
 */
template<typename SomeNode>
void receive(SomeNode& node, std::vector<Binding*>& bindings) {
  clearBindings(node, bindings.size());
  for (Count b = 0; b < bindings.size(); b++) {
    receive(node, *bindings[b]);
  }
}


/// ArrivalNode.

ArrivalNode::ArrivalNode(): arrival(new BindingArrival) {}

ArrivalNode::ArrivalNode(const ArrivalNode& other):
  Node(other), arrival(other.arrival)
{
  // We are another client for these arrivals.
  arrival->acquire();
}

ArrivalNode::~ArrivalNode() {
  arrival->release();
}

void clearBindings(ArrivalNode& node, Count reserved) {
  node.arrival->bindings.clear();
  node.arrival->bindings.reserve(reserved);
}

void ArrivalNode::propagateTo(Node& node) {
  node.propagate(arrival->bindings);
}


/// Binding.

Binding::Binding() {}

Binding::Binding(const Binding& $previous, const void* entity):
  previous(&$previous), entityOrSample(entity) {}

Binding::Binding(const Sample& sample): previous(0), entityOrSample(&sample) {}

Count Binding::count() const {
  Count count(0);
  for (
    const Binding* binding(this); binding->previous; binding = binding->previous
  ) {
    count++;
  }
  return count;
}

const bool Binding::entities(std::vector<const void*>& buffer) const {
  bool allGood(true);
  Count count = this->count();
  buffer.clear();
  buffer.resize(count);
  for (
    const Binding* binding(this); binding->previous; binding = binding->previous
  ) {
    // Fill in reverse order, so subtract count as we go. Also, no need to check
    // bounds. We already made sure we had the right size.
    buffer[--count] = binding->entityOrSample;
    if (!binding->entityOrSample) {
      allGood = false;
    }
  }
  return allGood;
}

/**
 * The entity at the current level, or 0 if at the sample.
 */
const void* Binding::entity() const {
  return previous ? entityOrSample : 0;
}

const Sample& Binding::sample() const {
  const Binding* binding(this);
  while (binding->previous) {
    binding = binding->previous;
  }
  return *reinterpret_cast<const Sample*>(binding->entityOrSample);
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

void LeafNode::propagate(vector<Binding*>& bindings) {
  receive(*this, bindings);
}

void LeafNode::propagate(vector<Binding>& bindings) {
  receive(*this, bindings);
}

void receive(LeafNode& node, Binding& binding) {
  // No kids for leaves, so just accept the binding.
  node.arrival->bindings.push_back(&binding);
}


/// Node.

Node::Node(Id $id): id($id), $parent(0) {}

Node::Node(const Node& other): id(other.id), $parent(0) {
  for (
    vector<Node*>::const_iterator k(other.kids.begin());
    k != other.kids.end();
    k++
  ) {
    Node* kid = (*k)->copy();
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
  // Clear for clean data in case accidentally used.
  id = 0;
  $parent = 0;
}

Node* Node::findById(Node::Id id) {
  if (this->id == id) {
    return this;
  }
  for (vector<Node*>::iterator k = kids.begin(); k != kids.end(); k++) {
    Node* node = (*k)->findById(id);
    if (node) {
      return node;
    }
  }
  // No match found.
  return 0;
}

void Node::insertParent(Node& parent, Count index) {
  // Make sure we have a spot for this kid.
  if (index >= parent.kids.size()) {
    throw "No such kid index.";
  }
  // Put the new parent in the current parent.
  Node* currentParent(this->parent());
  if (currentParent) {
    vector<Node*>& siblings = currentParent->kids;
    vector<Node*>::iterator location =
      find(siblings.begin(), siblings.end(), this);
    if (location == siblings.end()) {
      throw "Node not in parent.";
    }
    parent.id = root()->generateId();
    parent.$parent = currentParent;
    *location = &parent;
  }
  // Purge any kid in the way, then put the new kid there.
  if (parent.kids[index]) {
    // TODO Do I really want joint required here?
    Joint joint;
    parent.kids[index]->purge(joint);
  }
  this->$parent = &parent;
  parent.kids[index] = this;
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
  return $parent;
}

void Node::propagate() {
  for (Count k = 0; k < kids.size(); k++) {
    propagateTo(*kids[k]);
  }
}

void Node::purge(Joint& joint) {
  joint.node = this->parent();
  if (joint.node) {
    vector<Node*>& siblings = joint.node->kids;
    // TODO Put this logic in separate method then call it: this->extract();
    // Remove doesn't work for me here. I guess I don't understand it.
    vector<Node*>::iterator self =
      find(siblings.begin(), siblings.end(), this);
    if (self == siblings.end()) {
      throw "Extracted node not in parent.";
    }
    *self = 0;
    joint.location = self;
    siblings.erase(self);
  }
  delete this;
}

void Node::replaceWith(Node& node) {
  // Put the new parent in the current parent.
  Node* currentParent(this->parent());
  if (currentParent) {
    vector<Node*>& siblings = currentParent->kids;
    vector<Node*>::iterator location =
      find(siblings.begin(), siblings.end(), this);
    if (location == siblings.end()) {
      throw "Node not in parent.";
    }
    // Assign ids for the new tree members.
    if (root()) {
      struct IdNodeVisitor: NodeVisitor {
        IdNodeVisitor(RootNode& $root): root($root) {}
        virtual void visit(Node& node, void* data) {
          node.id = root.generateId();
        }
        RootNode& root;
      };
      IdNodeVisitor visitor(*root());
      node.traverse(visitor);
    }
    // Put the node in place.
    node.$parent = currentParent;
    *location = &node;
  }
  // Clean out the old parent link.
  this->$parent = 0;
}

void Node::pushKid(Node& kid) {
  RootNode* root = this->root();
  if (root) {
    kid.id = root->generateId();
  }
  kid.$parent = this;
  kids.push_back(&kid);
}

RootNode* Node::root() {
  Node* highest = this;
  while (highest->parent()) {
    highest = highest->parent();
  }
  return dynamic_cast<RootNode*>(highest);
}

void Node::traverse(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
  accept(visitor, data);
  for (std::vector<Node*>::iterator k = kids.begin(); k != kids.end(); k++) {
    (*k)->traverse(visitor, data);
  }
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

void PredicateNode::propagate(vector<Binding*>& bindings) {
  receive(*this, bindings);
  // TODO Split and propagate.
}

void PredicateNode::propagate(vector<Binding>& bindings) {
  receive(*this, bindings);
  // TODO Split and propagate.
}

void receive(PredicateNode& node, Binding& binding) {
  // Record the binding.
  node.arrival->bindings.push_back(&binding);
}


/// RootNode.

RootNode::RootNode(const RootNode& other):
  StorageNode(other), entityType(other.entityType), nextId(other.nextId) {}

RootNode::RootNode(const Type& $entityType, Node* $kid):
  StorageNode(1), entityType($entityType), kid($kid), nextId(id + 1)
{
  // TODO Delete if we remove vector kids.
  if (kid) {
    pushKid(*kid);
  } else {
    kids.push_back(kid);
  }
}

void RootNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
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

void RootNode::propagate(vector<Binding*>& bindings) {
  receive(*this, bindings);
  propagateTo(*kids.front());
}

void RootNode::propagate(vector<Binding>& bindings) {
  receive(*this, bindings);
  propagateTo(*kids.front());
}

void RootNode::propagate(const std::vector<Sample>& samples) {
  // Reserve but don't clear.
  storage->bindings.clear();
  storage->bindings.reserve(samples.size());
  // Put the samples into bindings and propagate.
  for (
    std::vector<Sample>::const_iterator s = samples.begin();
    s != samples.end();
    s++
  ) {
    Binding binding(*s);
    cuncuno::receive(*this, binding);
  }
  propagateTo(*kids.front());
}

void receive(RootNode& node, Binding& binding) {
  // Just store the binding.
  node.storage->bindings.push_back(binding);
}


/// StorageNode.

StorageNode::StorageNode(Id id): Node(id), storage(new BindingStorage) {}

StorageNode::StorageNode(const StorageNode& other):
  Node(other), storage(other.storage)
{
  // We are another client for this storage.
  storage->acquire();
}

StorageNode::~StorageNode() {
  storage->release();
}

void clearBindings(StorageNode& node, Count reserved) {
  node.storage->bindings.clear();
  node.storage->bindings.reserve(reserved);
}

void StorageNode::propagateTo(Node& node) {
  node.propagate(storage->bindings);
}


/// VariableNode.

VariableNode::VariableNode(Node* $kid): kid($kid) {
  // TODO Remove this if we switch away from vector storage.
  if (kid) {
    pushKid(*kid);
  } else {
    kids.push_back(kid);
  }
}

VariableNode::VariableNode(const VariableNode& other): StorageNode(other) {}

void VariableNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

Node* VariableNode::copy() {
  return new VariableNode(*this);
}

void VariableNode::propagate(vector<Binding*>& bindings) {
  receive(*this, bindings);
  propagateTo(*kids.front());
}

void VariableNode::propagate(vector<Binding>& bindings) {
  receive(*this, bindings);
  propagateTo(*kids.front());
}

void receive(VariableNode& node, Binding& binding) {
  // See which entities are left in the sample for binding.
  bool anyAvailable(false);
  const vector<const void*>& entities(binding.sample().entities);
  for (Count e = 0; e < entities.size(); e++) {
    const void* entity(entities[e]);
    // See if this entity is still available.
    bool available(true);
    const Binding* current(&binding);
    while (current) {
      if (entity == binding.entity()) {
        available = false;
        break;
      }
      current = current->previous;
    }
    if (available) {
      // It is. Remember we got one and store it.
      anyAvailable = true;
      node.storage->bindings.push_back(Binding(binding, entity));
    }
  }
  // See if we need to bind a dummy.
  if (!anyAvailable) {
    node.storage->bindings.push_back(Binding(binding, 0));
  }
}


}
