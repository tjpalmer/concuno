#include <Eigen/Dense>
#include "learner.h"
#include <sstream>
#include "tree.h"

using namespace Eigen;
using namespace std;

namespace cuncuno {


struct TreeLearner: Worker {

  TreeLearner(RootNode& root, const vector<Sample>& samples);

  void findBestExpansion();

  /**
   * TODO Make based on n-ary functions of entities with metrics.
   */
  void split(LeafNode& leaf, const Attribute& attribute);

  /**
   * Updates the probabilities assigned to leaf nodes.
   */
  void updateProbabilities();

  /**
   * The tree root.
   */
  RootNode& root;

  /**
   * The samples to learn from.
   */
  const std::vector<Sample>& samples;

};


/// Learner.

Learner::Learner(): Worker("Learner") {}

void Learner::learn(const vector<Sample>& samples) {

  // Beginnings of tree learning.
  RootNode root(entityType);
  root.basicTree();
  TreeLearner treeLearner(root, samples);
  treeLearner.findBestExpansion();

  // Load labels first. Among other things, that tells us how many entities
  // there are.
  vector<bool> labels;
  for (auto s(samples.begin()); s != samples.end(); s++) {
    const Sample& sample(*s);
    for (Count e(0); e < sample.entities.size(); e++) {
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
    auto a(entityType.attributes.begin()); a != entityType.attributes.end(); a++
  ) {
    Attribute& attribute(**a);
    if (attribute.type == Type::$float() && attribute.count == 2) {
      size_t index(0);
      for (auto s(samples.begin()); s != samples.end(); s++) {
        const Sample& sample(*s);
        for (
          auto e(sample.entities.begin());
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

TreeLearner::TreeLearner(RootNode& $root, const vector<Sample>& $samples):
  Worker("TreeLearner"), root($root), samples($samples) {}

void TreeLearner::findBestExpansion() {
  updateProbabilities();
  std::vector<LeafNode*> leaves;
  root.leaves(leaves);
  for (auto l(leaves.begin()); l != leaves.end(); l++) {
    std::stringstream message;
    message << "Found a leaf with " << (*l)->bindings.size() << " bindings.";
    log(message.str());
  }
  if (leaves.empty()) {
    throw "no leaves to expand";
  }
  // TODO KS threshold on leaves. For now, just do first.
  LeafNode& leaf(*leaves.front());
  // TODO Sample from available predicates.
  // TODO Learn predicate priors. For now, assume fewer parameters better.
  // TODO Factored predicates allow priors on attributes/functions instead of
  // TODO just whole opaque predicates.
  // TODO Really for now, just go in arbitrary order.
  // TODO Can also add variable nodes up to arity. Assume fewer better.
  // TODO Count preceding var nodes.
  Count varCount(0);
  Node* node(&leaf);
  while (node) {
    if (dynamic_cast<VariableNode*>(node)) {
      varCount++;
    }
    node = node->parent();
  }
  stringstream message;
  message << "Var count before expansion: " << varCount;
  log(message.str());
  for (Count a(0); a < root.entityType.attributes.size(); a++) {
    const Attribute& attribute(*root.entityType.attributes[a]);
    // TODO Once we have arbitrary functions, pull arity from there.
    // TODO Loop arity outside functions.
    Count arity = 1;
    // TODO Loop through adding.
    split(leaf, attribute);
    // TODO Check quality.
    // TODO Restore leaf.
  }
  // TODO Try expansions of P, VP, VVP.
  // TODO How to express work units as continuations for placement in heap?
}

void TreeLearner::split(LeafNode& leaf, const Attribute& attribute) {
  // TODO Remove but don't destroy leaf.
}

void TreeLearner::updateProbabilities() {
  // Updates probabalities in leaves.
  struct ProbabilityUpdateVisitor: BindingsNodeVisitor, Worker {
    ProbabilityUpdateVisitor(): Worker("ProbabilityUpdateVisitor") {}
    virtual void visit(LeafNode& node, std::vector<Binding*>& bindings) {
      Float total(0);
      Float trues(0);
      for (auto b(bindings.begin()); b != bindings.end(); b++) {
        // TODO Change to be one vote per bag, not per binding!!!!!
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
  ProbabilityUpdateVisitor updater;
  root.propagate(updater, samples);
}


}
