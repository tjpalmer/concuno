#include "learner.h"
#include "tree.h"

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


/// LeafNode.

void LeafNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}


/// KidNode.

KidNode::KidNode(): $parent(0) {}

Node* KidNode::parent() {
  return $parent;
}


/// Node.

Node::~Node() {
  while (!kids.empty()) {
    delete kids.back();
    kids.pop_back();
  }
}

void Node::leaves(std::vector<LeafNode*>& buffer) {
  // TODO
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
      visitor.visit(node, bindings);
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
        std::vector<Node*>::iterator k = node.kids.begin();
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

void Node::traverse(NodeVisitor& visitor, void* data) {
  accept(visitor, data);
  for (std::vector<Node*>::iterator k = kids.begin(); k != kids.end(); k++) {
    (*k)->traverse(visitor, data);
  }
}


/// PredicateNode.

void PredicateNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}


/// RootNode.

RootNode::RootNode(const Type& $entityType): entityType($entityType) {}

void RootNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}

void RootNode::basicTree() {
  // TODO Check for already existing kids and if so then bail out (with or
  // TODO without error?).
  kids.push_back(new LeafNode);
}

void RootNode::bindingsPush(const std::vector<Sample>& samples) {
  bindings.reserve(samples.size());
  for (Count s = 0; s < samples.size(); s++) {
    bindings.push_back(Binding(samples[s]));
  }
}


/// VariableNode.

void VariableNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}


}
