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


/// Node.

Node::Node(const Node* $parent, Count $varDepth):
  parent($parent), varDepth($varDepth) {}

Tree::Tree(const Type& $entityType, const std::vector<Sample>& samples):
  Node(0,0), entityType($entityType)
{
  // Can't reserve space without some default constructor:
  bindingRoots.reserve(samples.size());
  for (Count s = 0; s < samples.size(); s++) {
    bindingRoots.push_back(Binding(samples[s]));
  }
}

}
