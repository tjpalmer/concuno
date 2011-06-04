#include <stdio.h>
#include "learn.h"


cnRootNode* cnExpandAtLeaf(cnLearner* learner, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
void cnUpdateLeafProbabilities(cnRootNode* root);


cnRootNode* cnExpandAtLeaf(cnLearner* learner, cnLeafNode* leaf) {
  cnRootNode* bestYet = NULL;
  cnRootNode* root = cnNodeRoot(&leaf->node);
  cnCount varDepth = cnNodeVarDepth(&leaf->node);
  // TODO Clone root and find leaf in new tree.
  // TODO Safe to assume functions already sorted by arity?
  cnListEachBegin(root->entityFunctions, cnEntityFunction, function) {
    if (function->inCount > varDepth) {
      cnCount varsAdded;
      cnCount varsNeeded = function->inCount - varDepth;
      printf(
        "Need %ld more vars for %s.\n", varsNeeded, cnStr(&function->name)
      );
      // TODO Add vars as needed.
      for (varsAdded = 0; varsAdded < varsNeeded; varsAdded++) {
        // TODO Add vars.
      }
    }
    // TODO Clone before adding split??
    // TODO Add split on function.
  } cnEnd;
  return bestYet;
}


void cnLearnerDispose(cnLearner* learner) {
  // Nothing yet.
}


void cnLearnerInit(cnLearner* learner) {
  // Nothing yet.
}


cnRootNode* cnLearnerLearn(cnLearner* learner, cnRootNode* initial) {
  cnLeafNode* leaf;
  cnList leaves;
  // Make sure leaf probs are up to date.
  // TODO Figure out the initial LL and such.
  cnUpdateLeafProbabilities(initial);
  /* TODO Loop this section. */ {
    /* Pick a leaf to expand. */ {
      // Get the leaves (over again, yes).
      cnListInit(&leaves, sizeof(cnLeafNode*));
      cnNodeLeaves(&initial->node, &leaves);
      if (leaves.count < 1) {
        printf("No leaves to expand.\n");
        return NULL;
      }
      // TODO Pick the best leaf instead of the first.
      leaf = *(cnLeafNode**)leaves.items;
      // Clean up list of leaves.
      cnListDispose(&leaves);
    }
    return cnExpandAtLeaf(learner, leaf);
  }
}


void cnUpdateLeafProbabilities(cnRootNode* root) {
  cnList leaves;
  cnListInit(&leaves, sizeof(cnLeafNode*));
  cnNodeLeaves(&root->node, &leaves);
  //printf("Found %ld leaves.\n", leaves.count);
  cnListEachBegin(&leaves, cnLeafNode*, leaf) {
    // TODO Find proper assignment for each leaf considering the others.
    cnCount posCount = 0;
    cnCount total = 0;
    cnBindingBagList* bindingBags = (*leaf)->node.bindingBagList;
    if (bindingBags) {
      total += bindingBags->bindingBags.count;
      cnListEachBegin(&bindingBags->bindingBags, cnBindingBag, bindingBag) {
        posCount += bindingBag->bag->label;
      } cnEnd;
    }
    (*leaf)->probability = posCount / (cnFloat)total;
    //printf("Leaf with prob: %lf\n", (*leaf)->probability);
  } cnEnd;
  cnListDispose(&leaves);
}
