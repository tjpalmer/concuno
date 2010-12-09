#include <algorithm>
#include <Eigen/Dense>
#include "learner.h"
#include <sstream>
#include "tree.h"

#include <iostream>

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
  for (
    vector<Sample>::const_iterator s(samples.begin()); s != samples.end(); s++
  ) {
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

TreeLearner::TreeLearner(RootNode& $root, const vector<Sample>& $samples):
  Worker("TreeLearner"), root($root), samples($samples) {}

void TreeLearner::findBestExpansion() {
  updateProbabilities();
  std::vector<LeafNode*> leaves;
  root.leaves(leaves);
  for (vector<LeafNode*>::iterator l(leaves.begin()); l != leaves.end(); l++) {
    LeafNode& leaf(**l);
    stringstream message;
    message
      << "Found leaf " << leaf.id <<  " with "
      << leaf.arrival->bindings.size()
      << " bindings.";
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
  // Count preceding var nodes.
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
  // TODO Determine max arity of functions.
  Count maxArity(1);
  // Loop on number of new vars to add. We'd rather not add them.
  RootNode candidateBase(root);
  Node* expansionNode(dynamic_cast<LeafNode*>(candidateBase.findById(leaf.id)));
  Joint expansionJoint;
  expansionNode->purge(expansionJoint);
  expansionNode = expansionJoint.node;
  for (Count newVarCount(0); newVarCount <= maxArity; newVarCount++) {
    if (newVarCount) {
      cout << "Adding var." << endl;
      // TODO expansionNode->insertParent<VariableNode>();
      // Add new var node, also calling it the new expansion node.
      VariableNode& varNode(*new VariableNode);
      expansionNode->pushKid(varNode);
      // TODO expansionNode->propagate();
      expansionNode = &varNode;
    }
    // {LogEntry entry(this); entry << "Expansion node: " << expansionNode;}
    // log << "Expansion node: " << expansionNode << endEntry;
    // log << "Expansion node: " << expansionNode << endl;
    stringstream message;
    message << "Expansion node: " << expansionNode;
    log(message.str());
    // Limit arity by available vars.
    // TODO Organize or sort functions by arity?
    Count currentMaxArity(std::min(varCount + newVarCount, maxArity));
    for (Count arity(1); arity <= currentMaxArity; arity++) {
      // TODO Constrain newVarCount vars.
      // TODO Loop on functions, not just attributes.
      for (Count a(0); a < root.entityType.attributes.size(); a++) {
        const Attribute& attribute(*root.entityType.attributes[a]);
        // TODO Check if the function matches the arity.
        // TODO Once we have arbitrary functions, pull arity from there.
        // TODO Change to expand?
        // TODO Change to expand on any node: split(candidateLeaf, attribute);
        // TODO Check quality.
        // TODO Restore leaf.
      }
    }
  }
  // TODO How to express work units as continuations for placement in heap?
}

void TreeLearner::split(LeafNode& leaf, const Attribute& attribute) {
  // TODO Remove but don't destroy leaf.
  // TODO Consider options of which vars to use in the function.
  // TODO Even for asymmetric functions, no need to check both directions if the
  // TODO vars haven't already been used for other predicates.
}

void TreeLearner::updateProbabilities() {
  // Updates probabalities in leaves.
  struct ProbabilityUpdateVisitor: BindingsNodeVisitor, Worker {
    ProbabilityUpdateVisitor(): Worker("ProbabilityUpdateVisitor") {}
    virtual void visit(LeafNode& node, std::vector<Binding*>& bindings) {
      Float total(0);
      Float trues(0);
      for (
        vector<Binding*>::iterator b(bindings.begin()); b != bindings.end(); b++
      ) {
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
