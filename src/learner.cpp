#include <Eigen/Dense>
#include "learner.h"
#include <sstream>
#include "tree.h"

using namespace Eigen;
using namespace std;

namespace cuncuno {


struct TreeLearner {

  TreeLearner(RootNode& root);

  void findBestExpansion();

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


/// Learner.

Learner::Learner(): Worker("Learner") {}

void Learner::learn(const vector<Sample>& samples) {

  // TODO Move the tree to being a reference parameter?
  RootNode tree(entityType);
  tree.bindingsPush(samples);
  TreeLearner treeLearner(tree);
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

TreeLearner::TreeLearner(RootNode& $root): node(&$root), root($root) {}

void TreeLearner::findBestExpansion() {
  vector<LeafNode*> leaves;
  root.leaves(leaves);
}

void TreeLearner::updateProbabilities() {
  // TODO
}


}
