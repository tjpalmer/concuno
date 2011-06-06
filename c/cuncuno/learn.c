#include <limits.h>
#include <stdio.h>
#include "learn.h"


typedef struct cnExpansion {

  cnLeafNode* leaf;

  cnCount newVarCount;

  cnList(cnIndex) varIndices;

} cnExpansion;


cnRootNode* cnExpandAtLeaf(cnLearner* learner, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
void cnUpdateLeafProbabilities(cnRootNode* root);


cnRootNode* cnExpandAtLeaf(cnLearner* learner, cnLeafNode* leaf) {
  cnCount varsAdded;
  cnRootNode* bestYet = NULL;
  cnRootNode* root = cnNodeRoot(&leaf->node);
  cnCount maxArity = 0;
  cnCount minArity = LONG_MAX;
  cnCount minNewVarCount;
  cnCount varDepth = cnNodeVarDepth(&leaf->node);

  // Find the min and max arity.
  // TODO Sort by arity? Or assume priority given by order?
  cnListEachBegin(root->entityFunctions, cnEntityFunction, function) {
    if (function->inCount < minArity) {
      minArity = function->inCount;
    }
    if (function->inCount > maxArity) {
      maxArity = function->inCount;
    }
  } cnEnd;
  if (minArity > maxArity) {
    // Should cover cases with no functions, at least.
    // TODO Just assert at least one function to start with?
    minArity = maxArity;
  }
  // We need at least enough new variables to cover the min arity.
  minNewVarCount = minArity - varDepth;
  if (minNewVarCount < 0) {
    // Already have more than we need.
    minNewVarCount = 0;
  }

  // Start added vars from low to high. Allow up to as many new vars as we have
  // arity for functions.
  for (varsAdded = minNewVarCount; varsAdded <= maxArity; varsAdded++) {
    cnVarNode* var = cnVarNodeCreate(cnTrue);
    // TODO Check for null var!
    cnNodeReplaceKid(&leaf->node, &var->node);
    // TODO Propagate.
    leaf = *(cnLeafNode**)cnNodeKids(&var->node);
    varDepth++;
    // TODO Loop on arities to guarantee increasing order by arity?
    cnListEachBegin(root->entityFunctions, cnEntityFunction, function) {
      if (function->inCount > varDepth) {
        printf(
          "Need %ld more vars for %s.\n",
          function->inCount - varDepth, cnStr(&function->name)
        );
        continue;
      }
      if (function->inCount < varsAdded) {
        // We've already added more vars than we need for this one.
        printf(
          "Added %ld too many vars for %s.\n",
          varsAdded - function->inCount, cnStr(&function->name)
        );
        continue;
      }
      // TODO Clone before adding split??
      // TODO Add split on function.
    } cnEnd;
  }

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
  cnList(cnLeafNode*) leaves;
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
  cnList(cnLeafNode*) leaves;
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
