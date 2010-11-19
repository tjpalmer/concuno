#include "tree.h"

namespace cuncuno {

Node::Node(const Node* $parent, Count $varDepth):
  parent($parent), varDepth($varDepth) {}

void Node::binding(Count index, Binding& binding) const {
  binding.sample = samples.at(index);
  binding.entities.clear();
  binding.entities.reserve(varDepth);
  for (Count e = 0, i = varDepth*index; e < varDepth; e++, i++) {
    binding.entities.push_back(entities.at(i));
  }
}

Count Node::bindingCount() const {
  return samples.size();
}

void Node::bindingPush(const Binding& binding) {
  if (binding.entities.size() != varDepth) {
    throw "wrong entity count";
  }
  samples.push_back(binding.sample);
  for (Count e = 0; e < varDepth; e++) {
    entities.push_back(binding.entities.at(e));
  }
}

Tree::Tree(const Type& $entityType, const std::vector<Sample>& s):
  Node(0,0), entityType($entityType), samples(s) {}

}
