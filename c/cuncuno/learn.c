#include <limits.h>
#include <stdio.h>
#include "learn.h"


/**
 * An expansion that replaces a leaf in the tree, adding a split that
 * optionally follows multiple new vars.
 */
typedef struct cnExpansion {

  /**
   * The function to use at the split node.
   */
  cnEntityFunction* function;

  /**
   * The leaf to expand.
   */
  cnLeafNode* leaf;

  /**
   * The number of new var nodes to add before the split.
   */
  cnCount newVarCount;

  /**
   * Which var indices (counting from the root down) to use in the split.
   */
  cnIndex* varIndices;

} cnExpansion;


/**
 * Returns a new tree with the expansion applied, optimized (so to speak)
 * according to the SMRF algorithm.
 *
 * TODO This might represent the best of multiple attempts at optimization.
 */
cnRootNode* cnExpandedTree(cnLearner* learner, cnExpansion* expansion);


cnBool cnPushExpansionsByIndices(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth
);


/**
 * Recursively push new expansions
 */
cnBool cnRecurseExpansions(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth,
  cnIndex* indices, cnCount filledCount
);


cnRootNode* cnTryExpansionsAtLeaf(cnLearner* learner, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
void cnUpdateLeafProbabilities(cnRootNode* root);


cnRootNode* cnExpandedTree(cnLearner* learner, cnExpansion* expansion) {
  // TODO Loop across multiple inits/attempts?
  cnCount varsAdded;
  for (varsAdded = 0; varsAdded < expansion->newVarCount; varsAdded++) {
    // TODO cnVarNode* var = cnVarNodeCreate(cnTrue);
    // TODO Check for null var!
    // TODO cnNodeReplaceKid(&leaf->node, &var->node);
    // TODO Propagate.
    // TODO leaf = *(cnLeafNode**)cnNodeKids(&var->node);
  }
  // TODO Split node.
  // TODO Optimize split.
  // TODO Return new tree.
  return NULL;
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
    return cnTryExpansionsAtLeaf(learner, leaf);
  }
}


cnBool cnLoopArity(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth,
  cnIndex* indices, cnIndex index, cnCount filledCount
) {
  cnIndex a;
  cnCount arity = prototype->function->inCount;
  for (a = 0; a < arity; a++) {
    if (indices[a] < 0) {
      indices[a] = index;
      if (!cnRecurseExpansions(
        expansions, prototype, varDepth, indices, filledCount + 1
      )) {
        return cnFalse;
      }
      indices[a] = -1;
    }
  }
  return cnTrue;
}

cnBool cnPushExpansionsByIndices(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth
) {
  // TODO Validate counts match up? Or just trust context since this function
  // TODO is private to this file?
  cnCount arity = prototype->function->inCount;
  cnIndex* index;
  cnIndex* indices = malloc(arity * sizeof(cnIndex));
  cnIndex* end = indices + arity;
  cnBool result;
  if (!indices) {
    printf("Failed to allocate indices.\n");
    return cnFalse;
  }
  // Init to -1 as "not assigned".
  for (index = indices; index < end; index++) {
    *index = -1;
  }
  // TODO Consider symmetry on functions for new vars?
  result = cnRecurseExpansions(
    expansions, prototype, varDepth, indices, varDepth - prototype->newVarCount
  );
  free(indices);
  return result;
}


cnBool cnRecurseExpansions(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth,
  cnIndex* indices, cnCount filledCount
) {
  cnIndex a;
  cnCount arity = prototype->function->inCount;
  cnIndex index = arity - prototype->newVarCount + filledCount;
  //printf("cnRecurseExpansions(..., %ld, ..., %ld)\n", varDepth, filledCount);
  //printf("Index: %ld\n", index);
  //printf("Indices: ");
  //for (a = 0; a < arity; a++) {
  //  printf("%ld%s ", indices[a], indices[a] == index ? "*" : "");
  //}
  //printf("\n");
  if (filledCount >= arity) {
    // All filled up. Push an expansion with the given indices.
    // TODO Consider symmetry on functions for new vars?
    if (!(prototype->varIndices = malloc(arity * sizeof(cnIndex)))) {
      return cnFalse;
    }
    if (!cnListPush(expansions, prototype)) {
      free(prototype->varIndices);
      return cnFalse;
    }
    // Revert the prototype.
    prototype->varIndices = NULL;
    //printf("Done.\n");
    return cnTrue;
  }
  // Check if committed are used up.
  if (index >= varDepth) {
    //printf("Past committed.\n");
    // Got to the end of committed vars, so go back to the beginning.
    cnCount uncommittedCount = varDepth - prototype->newVarCount;
    for (index = 0; index < uncommittedCount; index++) {
      // See if we've already used this index.
      // TODO Replace this loop on indices with something constant time?
      cnBool used = cnFalse;
      for (a = 0; a < arity; a++) {
        if (indices[a] == index) {
          used = cnTrue;
          break;
        }
      }
      if (!used) {
        // Index not used. Try all the remaining spots for it.
        // Note that the recursive call here is slightly different than for the
        // loop below on committed vars. That makes it awkward to combine in
        // one.
        if (!cnLoopArity(
          expansions, prototype, varDepth, indices, index, filledCount
        )) {
          return cnFalse;
        }
      }
    }
  } else {
    //printf("Still committed.\n");
    // Still on committed variables. This index gets a spot for sure.
    if (!cnLoopArity(
      expansions, prototype, varDepth, indices, index, filledCount
    )) {
      return cnFalse;
    }
  }
  return cnTrue;
}


cnRootNode* cnTryExpansionsAtLeaf(cnLearner* learner, cnLeafNode* leaf) {
  cnRootNode* bestYet = NULL;
  cnList(cnExpansion) expansions;
  cnCount maxArity = 0;
  cnCount minArity = LONG_MAX;
  cnCount minNewVarCount;
  cnCount newVarCount;
  cnRootNode* root = cnNodeRoot(&leaf->node);
  cnCount varDepth = cnNodeVarDepth(&leaf->node);

  // Make a list of expansions. They can then be sorted, etc.
  cnListInit(&expansions, sizeof(cnExpansion));

  // Find the min and max arity.
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
  for (newVarCount = minNewVarCount; newVarCount <= maxArity; newVarCount++) {
    cnExpansion expansion;
    varDepth++;
    cnListEachBegin(root->entityFunctions, cnEntityFunction, function) {
      if (function->inCount > varDepth) {
        printf(
          "Need %ld more vars for %s.\n",
          function->inCount - varDepth, cnStr(&function->name)
        );
        continue;
      }
      if (function->inCount < newVarCount) {
        // We've already added more vars than we need for this one.
        printf(
          "Added %ld too many vars for %s.\n",
          newVarCount - function->inCount, cnStr(&function->name)
        );
        continue;
      }
      // Init a prototype expansion, then push index permutations.
      expansion.function = function;
      expansion.leaf = leaf;
      expansion.newVarCount = newVarCount;
      expansion.varIndices = NULL;
      if (!cnPushExpansionsByIndices(&expansions, &expansion, varDepth)) {
        printf("Failed to push expansions.\n");
        goto DONE;
      }
    } cnEnd;
  }

  // TODO Sort by arity? Or assume priority given by order? Some kind of
  // TODO heuristic?
  printf("Need to try %ld expansions.\n", expansions.count);
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    cnRootNode* expanded = cnExpandedTree(learner, expansion);
    // TODO Evaluate LL to see if it's the best yet. If not ...
    cnNodeDispose((cnNode*)expanded);
  } cnEnd;

  DONE:
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    free(expansion->varIndices);
  } cnEnd;
  cnListDispose(&expansions);
  return bestYet;
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
