#include <Eigen/Dense>
#include "learner.h"
#include <sstream>
#include "tree.h"

using namespace Eigen;
using namespace std;

namespace cuncuno {


struct TreeLearner: Worker {

  TreeLearner(RootNode& root);

  void findBestExpansion();

  /**
   * Sometimes we have direct storage. If so, use this function.
   *
   * The visitor receives a vector of pointers to bindings as data.
   */
  void propagate(NodeVisitor& visitor, std::vector<Binding>& bindings);

  /**
   * Sometimes we have pointers.
   *
   * The visitor receives a vector of pointers to bindings as data.
   */
  void propagate(NodeVisitor& visitor, std::vector<Binding*>& bindings);

  /**
   * Updates the probabilities assigned to leaf nodes.
   */
  void updateProbabilities();

  /**
   * The current node under consideration.
   *
   * TODO Do we need this???
   */
  Node* node;

  /**
   * The tree root.
   */
  RootNode& root;

};


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


/// Learner.

Learner::Learner(): Worker("Learner") {}

void Learner::learn(const vector<Sample>& samples) {

  // TODO Move the tree to being a reference parameter?
  RootNode tree(entityType);
  tree.basicTree();
  std::vector<Binding> bindings;
  for (
    std::vector<Sample>::const_iterator s = samples.begin();
    s != samples.end();
    s++
  ) {
    bindings.push_back(Binding(*s));
  }
  struct ProbabilityUpdateVisitor: NodeVisitor, Worker {
    ProbabilityUpdateVisitor(): Worker("ProbabilityUpdateVisitor") {}
    virtual void visit(LeafNode& node, void* data) {
      std::vector<Binding*>&
        bindings(*reinterpret_cast<std::vector<Binding*>*>(data));
      Float total(0);
      Float trues(0);
      for (
        std::vector<Binding*>::iterator b(bindings.begin());
        b != bindings.end();
        b++
      ) {
        total++;
        trues += (*b)->sample().label ? 1 : 0;
      }
      node.probability = trues / total;
      // TODO Delete the log.
      stringstream message;
      message << "Leaf probability: " << node.probability;
      log(message.str());
    }
  };
  TreeLearner treeLearner(tree);
  ProbabilityUpdateVisitor updater;
  // TODO Move propagation to the tree itself.
  treeLearner.propagate(updater, bindings);
  // TODO The next line is pure bogus for now.
  treeLearner.findBestExpansion();

  // Load labels first. Among other things, that tells us how many entities
  // there are.
  vector<bool> labels;
  for (
    vector<Sample>::const_iterator s(samples.begin()); s != samples.end(); s++
  ) {
    const Sample& sample(*s);
    for (size_t e(0); e < sample.entities.size(); e++) {
      labels.push_back(sample.label);
    }
  }

  // TODO I need instantiation nodes and a more formed tree notion even to get
  // TODO this far legitimately.

  // Load up all the attribute values for each attribute.
  // TODO This handles individual values. N-tuples (and pairs) should recurse.
  // TODO That is, pairs need done per sample bag, not the whole list.
  Matrix<Float,Dynamic,2> values2D(MatrixXd::Zero(labels.size(),2));
  // TODO Figure out how to get data pointers from Eigen.
  // TODO Just one buffer large enough for all dimensions?
  Float buffer2D[2];
  for (
    vector<Attribute*>::iterator a(entityType.attributes.begin());
    a != entityType.attributes.end();
    a++
  ) {
    Attribute& attribute(**a);
    if (attribute.type == Type::$float() && attribute.count == 2) {
      size_t index(0);
      for (
        vector<Sample>::const_iterator s(samples.begin());
        s != samples.end();
        s++
      ) {
        const Sample& sample(*s);
        for (
          vector<const void*>::const_iterator e(sample.entities.begin());
          e != sample.entities.end();
          e++, index++
        ) {
          attribute.get(*e, buffer2D);
          // TODO There's probably some better way than two assignments.
          values2D(index,0) = buffer2D[0];
          values2D(index,1) = buffer2D[1];
        }
      }
      // A bit of logging status. TODO Are there easier ways than all this?
      stringstream message;
      //cout << values2D << endl;
      message
        << attribute.name << " values loaded: " << values2D.rows() << endl
      ;
      log(message.str());
    }
  }

}

//struct Learner: Learner


/// TreeLearner

TreeLearner::TreeLearner(RootNode& $root):
  Worker("TreeLearner"), node(&$root), root($root) {}

void TreeLearner::findBestExpansion() {
  vector<LeafNode*> leaves;
  root.leaves(leaves);
}

void TreeLearner::propagate(
    NodeVisitor& visitor, std::vector<Binding>& bindings
) {
  std::vector<Binding*> pointers;
  pushPointers(bindings, pointers);
  propagate(visitor, pointers);
}

void TreeLearner::propagate(
  NodeVisitor& visitor, std::vector<Binding*>& bindings
) {
  struct PropagateVisitor: NodeVisitor, Worker {
    std::vector<Binding*>& cast(void* data) {
      return *reinterpret_cast<std::vector<Binding*>*>(data);
    }
    PropagateVisitor(NodeVisitor& $visitor):
      Worker("PropagateVisitor"), visitor($visitor) {}
    virtual void visit(LeafNode& node, void* data) {
      visitor.visit(node, data);
    }
    virtual void visit(PredicateNode& node, void* data) {
      visitor.visit(node, data);
      // TODO std::vector<Binding*>& incoming(cast(data));
      // TODO Split on predicate.
    }
    virtual void visit(VariableNode& node, void* data) {
      visitor.visit(node, data);
      // TODO std::vector<Binding*>& incoming(cast(data));
      // TODO Bind entities to variables.
    }
    virtual void visit(RootNode& node, void* data) {
      visitor.visit(node, data);
      for (
        std::vector<Node*>::iterator k = node.kids.begin();
        k != node.kids.end();
        k++
      ) {
        (*k)->accept(*this, data);
      }
    }
    NodeVisitor& visitor;
  };
  PropagateVisitor propagator(visitor);
  root.accept(propagator, &bindings);
}

void TreeLearner::updateProbabilities() {
  // TODO
}


}
