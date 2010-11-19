#include "tree.h"

using namespace std;

namespace cuncuno {

Node::Node(const Type& entityType): bindings(entityType) {}

Tree::Tree(const Type& entityType, const std::vector<Sample>& s):
  Node(entityType), samples(s) {}

}
