#include "learner.h"
#include "tree.h"

namespace cuncuno {


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

Node* Node::parent() {
  return 0;
}


/// NodeVisitor.

// Do nothing by default for all of these. That way, subtypes can ignore some.
void NodeVisitor::visit(LeafNode& node, void* data) {}
void NodeVisitor::visit(PredicateNode& node, void* data) {}
void NodeVisitor::visit(VariableNode& node, void* data) {}
void NodeVisitor::visit(RootNode& node, void* data) {}


/// PredicateNode.

void PredicateNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
}


/// RootNode.

RootNode::RootNode(const Type& $entityType): entityType($entityType) {}

void RootNode::accept(NodeVisitor& visitor, void* data) {
  visitor.visit(*this, data);
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
