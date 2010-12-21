#include "learner.h"
#include <sstream>
#include "tree.h"

#include <iostream>

using namespace std;

namespace cuncuno {


struct TreeLearner: Worker {

  TreeLearner(Learner& learner, RootNode& root, const vector<Sample>& samples);

  void findBestExpansion();

  void optimizePredicate(PredicateNode& splitter);

  /**
   * TODO Make based on n-ary functions of entities with metrics.
   */
  void split(LeafNode& leaf, const Function& function);

  /**
   * Updates the probabilities assigned to leaf nodes.
   */
  void updateProbabilities(RootNode& root);

  Learner& learner;

  /**
   * The tree root.
   */
  RootNode& root;

  /**
   * The samples to learn from.
   */
  const std::vector<Sample>& samples;

};


/**
 * Just calculate diverse density for now, based on Gaussian and Euclidean.
 *
 * TODO Figure out what this really should be doing.
 *
 * TODO Base this on a probability distribution or something.
 */
void diverseDensity(
  const vector<bool>& labels, const Float* values, Count ndim
);


/// Learner.

Learner::Learner(const Type& $entityType):
  Worker("Learner"), entityType($entityType) {}

void Learner::learn(const vector<Sample>& samples) {
  // Beginnings of tree learning.
  // TODO How to organize types to allow priority queue or whatever convenience?
  RootNode root(entityType);
  TreeLearner treeLearner(*this, root, samples);
  treeLearner.findBestExpansion();
}

//struct Learner: Learner


/// TreeLearner

TreeLearner::TreeLearner(
  Learner& $learner, RootNode& $root, const vector<Sample>& $samples
):
  Worker("TreeLearner"), learner($learner), root($root), samples($samples) {}

void TreeLearner::findBestExpansion() {
  // Propagate the samples and update probabilities.
  root.propagate(samples);
  updateProbabilities(root);
  // Pick a leaf to expand.
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
  Count maxArity(2);
  // Loop on number of new vars to add. We'd rather not add them.
  RootNode candidateTree(root);
  LeafNode* expansionNode(
    dynamic_cast<LeafNode*>(candidateTree.findById(leaf.id))
  );
  //Joint expansionJoint;
  //expansionNode->purge(expansionJoint);
  //expansionNode = expansionJoint.node;
  for (Count newVarCount(0); newVarCount <= maxArity; newVarCount++) {
    if (newVarCount) {
      // Add new var node.
      cout << "Adding var." << endl;
      VariableNode& varNode(*new VariableNode);
      // Steal the bindings for propagating. The leaf will be changing, but
      // other clones of the tree might depend on these.
      // TODO Protect against leaks from exceptions here!!!
      // TODO Investigate shared_ptr???
      expansionNode->replaceWith(varNode);
      {
        auto_ptr<LeafNode> autoDelete(expansionNode);
        // We want the exact set of bindings that the leaf already had. This
        // matters especially for predicate nodes, so we don't need to retest.
        expansionNode->propagateTo(varNode);
      }
      // The var node's leaf is the new expansion node.
      expansionNode = dynamic_cast<LeafNode*>(varNode.kids.front());
      // Update probabilities again already for kicks. TODO Delete this?
      updateProbabilities(candidateTree);
    }
    // {LogEntry entry(this); entry << "Expansion node: " << expansionNode;}
    // log << "Expansion node: " << expansionNode << endEntry;
    // log << "Expansion node: " << expansionNode << endl;
    stringstream message;
    message << "Expansion node: " << expansionNode;
    log(message.str());
    // Limit arity by available vars.
    // TODO Organize or sort functions by arity?
    Count currentMaxArity(min(varCount + newVarCount, maxArity));
    for (Count arity(newVarCount); arity <= currentMaxArity; arity++) {
      // TODO Constrain newVarCount vars.
      // TODO Loop on functions, not just attributes.
      for (Count f(0); f < learner.functions.size(); f++) {
        const Function& function(*learner.functions[f]);
        if (function.typeIn().count == arity) {
          RootNode tempTree(candidateTree);
          LeafNode& tempLeaf(
            *dynamic_cast<LeafNode*>(tempTree.findById(expansionNode->id))
          );
          split(tempLeaf, function);
          // TODO Check quality.
          // TODO Update candidate tree if better.
        }
      }
    }
  }
  // TODO How to express work units as continuations for placement in heap?
}

void TreeLearner::optimizePredicate(PredicateNode& splitter) {
  const Function& function(*splitter.predicate->function);
  vector<Binding*>& bindings(splitter.arrival->bindings);
  Count bindingCount(bindings.size());
  // Keep count of good bindings. We don't know which have dummies until we
  // go through them.
  // TODO Consider splitting out at var nodes to avoid this.
  // TODO Could also go through to count good vs. bad before allocating the
  // TODO values buffer.
  Count goodCount(0);
  vector<const void*> entities;
  Size valueSize(function.typeOut().size);
  vector<const void*> inBuffer(function.typeIn().count);
  vector<bool> labels(bindingCount);
  vector<Byte> values(valueSize * bindingCount);
  for (Count b(0); b < bindingCount; b++) {
    Binding& binding(*bindings[b]);
    // TODO Actually, we still need to know which to use.
    binding.entities(entities);
    bool allThere(true);
    // Pull out the specified arguments.
    for (Count a(0); a < splitter.args.size(); a++) {
      const void* entity(entities[splitter.args[a]]);
      if (!entity) {
        // One of the required arguments wasn't defined.
        allThere = false;
        break;
      }
      inBuffer[a] = entity;
    }
    if (allThere) {
      // Call the function, and call it good (for now).
      // TODO Some way to track errors here, too.
      // TODO Return true/false or throw exceptions?
      function(&inBuffer.front(), &values[valueSize * goodCount]);
      // TODO Anything more efficient than making a new list of labels?
      labels[goodCount] = binding.sample().label;
      goodCount++;
    } else {
      // TODO Track this one for error branch.
    }
  }
  // Don't claim we have more data than we do. Might not matter much either way.
  values.resize(valueSize * goodCount);
  labels.resize(goodCount);
  cout
    << "Calculated " << goodCount << " (from " << bindingCount << ")"
    << " values of size " << function.typeOut().size << endl;
  if (function.typeOut().base == function.typeOut().system.$float()) {
    diverseDensity(
      labels,
      reinterpret_cast<Float*>(&values.front()),
      function.typeOut().count
    );
  }
}

void TreeLearner::split(LeafNode& leaf, const Function& function) {
  cout << "Splitting with " << function.name << endl;
  RootNode splitTree(*leaf.root());
  LeafNode* expansionNode(
    dynamic_cast<LeafNode*>(splitTree.findById(leaf.id))
  );
  PredicateNode& splitter(*new PredicateNode);
  // TODO Generify this replace/propagate/delete logic?
  expansionNode->replaceWith(splitter);
  {
    auto_ptr<LeafNode> autoDelete(expansionNode);
    // TODO Without a limit on propagation to kids, this will be wasteful.
    expansionNode->propagateTo(splitter);
  }
  // Set up the predicate, except for the arg indexes.
  splitter.predicate = new FunctionPredicate;
  splitter.predicate->function = &function;
  // TODO Auto-manage args count?
  Count arity(splitter.predicate->function->typeIn().count);
  splitter.args.resize(arity);
  Count varCount = splitter.varCount();
  // TODO Go through the various argument options. This is fake for now.
  for (Count argOptionIndex(0); argOptionIndex < 1; argOptionIndex++) {
    // TODO Set args on the splitter.
    // TODO Even for asymmetric functions, no need to check both directions if the
    // TODO vars haven't already been used for other predicates.
    // For now, just use the most recent vars with the most recent at the end.
    for (Count a(0); a < arity; a++) {
      splitter.args[a] = varCount - arity + a;
    }
    // TODO Optimization could be kicked off into a heap here, or perhaps at any
    // TODO hierarchical level.
    optimizePredicate(splitter);
  }
}

void TreeLearner::updateProbabilities(RootNode& root) {
  // Updates probabalities in leaves.
  vector<LeafNode*> leaves;
  root.leaves(leaves);
  for (Count l = 0; l < leaves.size(); l++) {
    LeafNode& leaf(*leaves[l]);
    cout << "Leaf " << leaf.id << flush;
    vector<Binding*>& bindings(leaf.arrival->bindings);
    Float total(0);
    Float trues(0);
    for (
      vector<Binding*>::iterator b(bindings.begin()); b != bindings.end(); b++
    ) {
      // TODO Change to be one vote per bag, not per binding!!!!!
      // TODO Actually, we do need to allow some bindings in true bags to be
      // TODO counted as false. Optimize across leaf probabilities.
      total++;
      trues += (*b)->sample().label ? 1 : 0;
    }
    leaf.probability = trues / total;
    // TODO Delete the log.
    cout
      << " probability: " << leaf.probability
      << " (" << trues << "/" << total << ")" << endl
    ;
  }
}


void diverseDensity(
  const vector<bool>& labels, const Float* values, Count ndim
) {
  // TODO Real work.
  cout << "Calculate " << ndim << "D diverse density" << endl;
}


}
