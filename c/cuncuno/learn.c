#include <limits.h>
#include <stdio.h>
#include <string.h>
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


cnBool cnLearnSplitModel(cnLearner* learner, cnSplitNode* split);


void cnPrintExpansion(cnExpansion* expansion);


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
cnBool cnUpdateLeafProbabilities(cnRootNode* root);


cnRootNode* cnExpandedTree(cnLearner* learner, cnExpansion* expansion) {
  // TODO Loop across multiple inits/attempts?
  cnLeafNode* leaf;
  cnNode* parent;
  cnSplitNode* split;
  cnRootNode* root = cnNodeRoot(&expansion->leaf->node);
  cnCount varsAdded;
  printf("Expanding on "); cnPrintExpansion(expansion);

  // Create a copied tree to work with.
  root = (cnRootNode*)cnTreeCopy(&root->node);
  if (!root) {
    printf("No tree copy for expansion.\n");
    return NULL;
  }
  leaf = (cnLeafNode*)cnNodeFindById(&root->node, expansion->leaf->node.id);
  parent = leaf->node.parent;

  // Add requested vars.
  for (varsAdded = 0; varsAdded < expansion->newVarCount; varsAdded++) {
    cnVarNode* var = cnVarNodeCreate(cnTrue);
    if (!var) {
      printf("No var %ld for expansion.\n", varsAdded);
      goto ERROR;
    }
    cnNodeReplaceKid(&leaf->node, &var->node);
    leaf = *(cnLeafNode**)cnNodeKids(&var->node);
  }

  // Add the split, and provide the bindings from the old parent.
  // TODO Just always propagate from the root?
  // TODO Don't bother even to store bindings??? How fast is it, usually?
  if (!(split = cnSplitNodeCreate(cnTrue))) {
    goto ERROR;
  }
  cnNodeReplaceKid(&leaf->node, &split->node);
  cnNodePropagate(parent, NULL);

  // Configure the split, and learn a model (distribution, threshold).
  split->function = expansion->function;
  // Could just borrow the indices, but that makes management more complicated.
  // TODO Array malloc/copy function?
  split->varIndices = malloc(split->function->inCount * sizeof(cnIndex));
  if (!split->varIndices) {
    printf("No var indices for expansion.\n");
    goto ERROR;
  }
  memcpy(
    split->varIndices, expansion->varIndices,
    split->function->inCount * sizeof(cnIndex)
  );
  if (!cnLearnSplitModel(learner, split)) {
    printf("No split learned for expansion.\n");
    goto ERROR;
  }

  // Return the new tree.
  return root;

  ERROR:
  // Just destroy the whole tree copy for now in case of any error.
  cnNodeDrop(&root->node);
  return NULL;
}


void cnLearnerDispose(cnLearner* learner) {
  // Nothing yet.
}


void cnLearnerInit(cnLearner* learner) {
  // Nothing yet.
}


cnBool cnLearnSplitModel(cnLearner* learner, cnSplitNode* split) {
  return cnTrue;
}


cnRootNode* cnLearnTree(cnLearner* learner, cnRootNode* initial) {
  cnLeafNode* leaf;
  cnList(cnLeafNode*) leaves;
  // Make sure leaf probs are up to date.
  // TODO Figure out the initial LL and such.
  if (!cnUpdateLeafProbabilities(initial)) {
    printf("Failed to update leaf probabilities.\n");
    return NULL;
  }
  /* TODO Loop this section. */ {
    /* Pick a leaf to expand. */ {
      // Get the leaves (over again, yes).
      cnListInit(&leaves, sizeof(cnLeafNode*));
      if (!cnNodeLeaves(&initial->node, &leaves)) {
        printf("Failed to gather leaves.\n");
        return NULL;
      }
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


void cnPrintExpansion(cnExpansion* expansion) {
  cnIndex i;
  printf("%s(", cnStr(&expansion->function->name));
  for (i = 0; i < expansion->function->inCount; i++) {
    if (i > 0) {
      printf(", ");
    }
    printf("%ld", expansion->varIndices[i]);
  }
  printf(
    ") at node %ld with %ld new vars.\n",
    expansion->leaf->node.id, expansion->newVarCount
  );
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
    memcpy(prototype->varIndices, indices, arity * sizeof(cnIndex));
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
        //printf(
        //  "Need %ld more vars for %s.\n",
        //  function->inCount - varDepth, cnStr(&function->name)
        //);
        continue;
      }
      if (function->inCount < newVarCount) {
        // We've already added more vars than we need for this one.
        //printf(
        //  "Added %ld too many vars for %s.\n",
        //  newVarCount - function->inCount, cnStr(&function->name)
        //);
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
    if (!expanded) {
      goto DONE;
    }
    // TODO Evaluate LL to see if it's the best yet. If not ...
    // TODO Consider paired randomization test with validation set for
    // TODO significance test.
    cnNodeDrop(&expanded->node);
  } cnEnd;

  DONE:
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    free(expansion->varIndices);
  } cnEnd;
  cnListDispose(&expansions);
  return bestYet;
}


cnBool cnUpdateLeafProbabilities(cnRootNode* root) {
  cnBool result = cnTrue;
  cnList(cnLeafNode*) leaves;
  cnListInit(&leaves, sizeof(cnLeafNode*));
  if (!cnNodeLeaves(&root->node, &leaves)) {
    result = cnFalse;
    goto DONE;
  }
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
  DONE:
  cnListDispose(&leaves);
  return result;
}
