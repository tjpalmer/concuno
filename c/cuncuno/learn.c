#include <stdio.h>
#include "learn.h"


/**
 * Updates all the leaf probabilities in the tree.
 */
void cnUpdateLeafProbabilities(cnRootNode* root);


void cnLearnerDispose(cnLearner* learner) {
  // Nothing yet.
}


void cnLearnerInit(cnLearner* learner) {
  // Nothing yet.
}


cnRootNode* cnLearnerLearn(cnLearner* learner, cnRootNode* initial) {
  // TODO Figure out the initial LL and such.
  cnList leaves;
  cnUpdateLeafProbabilities(initial);
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
