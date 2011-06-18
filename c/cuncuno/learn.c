#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "learn.h"
#include "mat.h"
#include "stats.h"


/**
 * Tracks the distance to a bag from some point.
 */
typedef struct cnBagDistance {

  /**
   * The bag in question, in case you want details (like the label).
   */
  cnPointBag* bag;

  /**
   * The farthest distance in this bag.
   */
  cnFloat far;

  /**
   * The nearest distance in this bag.
   */
  cnFloat near;

} cnBagDistance;


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
 * Hack to get the best point based on pseudo diverse density.
 *
 * TODO Provide more than one starting point? Or will KS flood fill be best
 * TODO for deciding what's too close or not?
 */
cnFloat* cnBestPointByDiverseDensity(
  cnTopology topology, cnList(cnPointBag)* pointBags
);


/**
 * Choose a threshold on the following distances to maximize the "noisy-and
 * noisy-or" metric, assuming that this determination is the only thing that
 * matters (no leaves elsewhere).
 */
cnFloat cnChooseThreshold(
  cnBagDistance* distances, cnBagDistance* distancesEnd
);


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


/**
 * Perform a fill search by expanding isotropically from each point, limiting
 * by the metric, going to furthest points contained and spreading them. Min
 * distance from control points to each bag are maintained.
 *
 * TODO Provide a list of all contained positive points at end.
 */
void cnSearchFill(
  cnTopology topology, cnList(cnPointBag)* pointBags, cnFloat* searchStart
);


cnRootNode* cnTryExpansionsAtLeaf(cnLearner* learner, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
cnBool cnUpdateLeafProbabilities(cnRootNode* root);


void cnBuildInitialKernel(cnTopology topology, cnList(cnPointBag)* pointBags) {
  cnFloat* positiveVector;
  cnFloat* positiveVectors = NULL;
  cnCount positiveVectorCount;
  cnFloat* matrixEnd;
  cnCount valueCount = 0; // per vector.
  cnFloat* vector;

  if (topology != cnTopologyEuclidean) {
    printf("I handle only Euclidean right now, not %u.\n", topology);
    return;
  }

  // Count good, positive vectors.
  positiveVectorCount = 0;
  printf("pointBags->count: %ld\n", pointBags->count);
  if (pointBags->count > 0) {
    valueCount = ((cnPointBag*)pointBags->items)->valueCount;
  }
  printf("valueCount: %ld\n", valueCount);
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    vector = pointBag->pointMatrix;
    matrixEnd = vector + pointBag->pointCount * valueCount;
    for (; vector < matrixEnd; vector += valueCount) {
      cnBool allGood = cnTrue;
      cnFloat* value = vector;
      cnFloat* vectorEnd = vector + valueCount;
      for (value = vector; value < vectorEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
      }
      positiveVectorCount += allGood;
    }
  } cnEnd;

  if (!positiveVectorCount) {
    printf("I need vectors to work with!\n");
    return;
  }

  // Make a matrix of the positive, good vectors.
  positiveVectors = malloc(positiveVectorCount * valueCount * sizeof(cnFloat));
  if (!positiveVectors) {
    goto DONE;
  }
  positiveVector = positiveVectors;
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    vector = pointBag->pointMatrix;
    matrixEnd = vector + pointBag->pointCount * valueCount;
    for (; vector < matrixEnd; vector += valueCount) {
      cnBool allGood = cnTrue;
      cnFloat *value = vector, *positiveValue = positiveVector;
      cnFloat* vectorEnd = vector + valueCount;
      for (value = vector; value < vectorEnd; value++, positiveValue++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
        *positiveValue = *value;
      }
      if (allGood) {
        // Move to the next.
        positiveVector += valueCount;
      }
    }
  } cnEnd;

  printf(
    "Got a %ld by %ld matrix of positive vectors.\n",
    valueCount, positiveVectorCount
  );
  {
    // TODO Actually create a Gaussian PDF.
    cnFloat* stat = malloc(valueCount * sizeof(cnFloat));
    if (!stat) goto DONE;
    printf("Max, mean of positives: ");
    cnVectorPrint(
      valueCount,
      cnVectorMax(valueCount, stat, positiveVectorCount, positiveVectors)
    );
    printf(", ");
    cnVectorPrint(
      valueCount,
      cnVectorMean(valueCount, stat, positiveVectorCount, positiveVectors)
    );
    printf("\n");
    free(stat);
  }

  DONE:
  free(positiveVectors);
}


cnFloat* cnBestPointByDiverseDensity(
  cnTopology topology, cnList(cnPointBag)* pointBags
) {
  cnFloat* bestPoint = NULL;
  cnFloat bestSumYet = HUGE_VAL;
  cnCount posBagCount = 0, maxPosBags = 4;
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;
  printf("DD-ish: ");
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    cnFloat* vector = pointBag->pointMatrix;
    cnFloat* matrixEnd = vector + pointBag->pointCount * valueCount;
    if (!pointBag->bag->label) continue;
    if (posBagCount++ >= maxPosBags) break;
    printf("B ");
    for (; vector < matrixEnd; vector += valueCount) {
      cnFloat sumNegMin = 0, sumPosMin = 0;
      cnBool allGood = cnTrue;
      cnFloat* value;
      cnFloat* vectorEnd = vector + valueCount;
      for (value = vector; value < vectorEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
      }
      if (!allGood) continue;
      cnListEachBegin(pointBags, cnPointBag, pointBag2) {
        cnFloat minDistance = HUGE_VAL;
        cnFloat* vector2 = pointBag2->pointMatrix;
        cnFloat* matrixEnd2 = vector2 + pointBag2->pointCount * valueCount;
        for (; vector2 < matrixEnd2; vector2 += valueCount) {
          cnFloat distance = 0;
          cnFloat* value2;
          for (
            value = vector, value2 = vector2;
            value < vectorEnd;
            value++, value2++
          ) {
            cnFloat diff = *value - *value2;
            distance += diff * diff;
          }
          if (distance < minDistance) {
            minDistance = distance;
          }
        }
        // TODO Base on specialized kernel?
        // TODO Support non-Euclidean topologies.
        if (minDistance < HUGE_VAL) {
          if (pointBag2->bag->label) {
            // This is actually a negative log probability.
            sumPosMin += minDistance;
          } else {
            // Further is better for negatives.
            minDistance = -log(1 - exp(-minDistance));
            sumNegMin += minDistance;
          }
        }
      } cnEnd;
      // Print and check.
      if (sumPosMin + sumNegMin < bestSumYet) {
        printf("(");
        cnVectorPrint(valueCount, vector);
        printf(": %.4le) ", sumPosMin + sumNegMin);
        bestPoint = vector;
        bestSumYet = sumPosMin + sumNegMin;
      }
    }
  } cnEnd;
  printf("\n");
  return bestPoint;
}


/**
 * We need to track near and far distances in a nice sorted list. This lets us
 * do that.
 */
typedef enum {
  cnChooseThreshold_Both, cnChooseThreshold_Far, cnChooseThreshold_Near
} cnChooseThreshold_Edge;

typedef struct {
  cnBagDistance* distance;
  cnChooseThreshold_Edge edge;
} cnChooseThreshold_Distance;

cnFloat cnChooseThreshold_EdgeDist(const cnChooseThreshold_Distance* dist) {
  // The distances themselves. For both, we can use either.
  return dist->edge == cnChooseThreshold_Near ?
    dist->distance->near : dist->distance->far;
}

int cnChooseThreshold_Compare(const void* a, const void* b) {
  cnFloat distA = cnChooseThreshold_EdgeDist(a);
  cnFloat distB = cnChooseThreshold_EdgeDist(b);
  return distA > distB ? 1 : distA == distB ? 0 : -1;
}

cnFloat cnChooseThreshold(
  cnBagDistance* distances, cnBagDistance* distancesEnd
) {
  // TODO Just allow sorting the original data instead of malloc here?
  cnCount bagCount = distancesEnd - distances;
  cnBagDistance* distance;
  cnChooseThreshold_Distance* dists =
    malloc(2 * bagCount * sizeof(cnChooseThreshold_Distance));
  cnChooseThreshold_Distance* dist;
  cnChooseThreshold_Distance* distsEnd = dists + 2 * bagCount;
  cnCount negBothCount = 0; // Negatives on both sides of the threshold.
  cnCount negFalseCount = 0; // Negatives fully outside.
  cnCount negTotalCount = 0; // Total count of negatives.
  cnCount negTrueCount = 0; // Negatives fully inside.
  cnCount posBothCount = 0; // Positives on both sides.
  cnCount posFalseCount = 0; // Positives fully outside.
  cnCount posTotalCount = 0; // Total count of positives.
  cnCount posTrueCount = 0; // Positives fully inside.
  cnCount negAsTrueCount, negAsFalseCount; // Neg count effectively in or out.
  cnCount posAsTrueCount, posAsFalseCount; // Pos count effectively in or out.
  cnCount trueCount, falseCount; // Count of bags inside or outside.
  cnCount bestTrueCount = 0, bestFalseCount = 0;
  cnFloat bestMetric = -HUGE_VAL;
  cnFloat metric;
  cnFloat trueProb, bestTrueProb = cnNaN();
  cnFloat falseProb, bestFalseProb = cnNaN();
  cnFloat threshold = 0;

  if (!dists) {
    // No good.
    printf("Failed to allocate dists.\n");
    return cnNaN();
  }

  // Init the pointer array.
  dist = dists;
  for (distance = distances; distance < distancesEnd; distance++) {
    if (distance->near < HUGE_VAL) {
      // It's not an error case, so include it.
      if (distance->bag->bag->label) {
        // Count the positives.
        posTotalCount++;
      }
      // Reference the distance, and move on.
      dist->distance = distance;
      if (distance->near == distance->far) {
        dist->edge = cnChooseThreshold_Both;
        // In these cases, we don't store both sides. Just a single "Both".
        // Technically, we could, but complication just shifts. In this case,
        // it would move to the compare function that would need to make sure
        // to sort nears before fars.
        distsEnd--;
      } else {
        // Put in the near side.
        dist->edge = cnChooseThreshold_Near;
        // And the far.
        dist++;
        dist->distance = distance;
        dist->edge = cnChooseThreshold_Far;
      }
      // Move on.
      dist++;
    } else {
      // Error case. Leave out both near and far ends.
      // We might have allocated more space than needed for dists, but it's not
      // likely a huge gap, unless we have lots of dummy bindings.
      // TODO Is err probability relevant to the grand metric here?
      // TODO If so, count pos and neg errs.
      distsEnd -= 2;
      bagCount--;
    }
  }
  // Update the count now to the ones we care about.
  negTotalCount = bagCount - posTotalCount;
  // And they are all outside the threshold so far.
  posFalseCount = posTotalCount;
  negFalseCount = negTotalCount;
  printf("Totals: %ld %ld (%d %ld)\n", posTotalCount, negTotalCount, distsEnd - dists, bagCount);

  // Sort it.
  qsort(
    dists, distsEnd - dists,
    sizeof(cnChooseThreshold_Distance), cnChooseThreshold_Compare
  );

  // Now go through the list, calculating effective probabilities and the
  // decision metric to find the optimal threshold.
  for (dist = dists; dist < distsEnd; dist++) {
    if (dist->distance->bag->bag->label) {
      // Change positive counts depending on leading, trailing, or both.
      switch (dist->edge) {
      case cnChooseThreshold_Both:
        posFalseCount--;
        posTrueCount++;
        break;
      case cnChooseThreshold_Far:
        posBothCount--;
        posTrueCount++;
        break;
      case cnChooseThreshold_Near:
        posBothCount++;
        posFalseCount--;
        break;
      }
    } else {
      // Change negative counts depending on leading, trailing, or both.
      switch (dist->edge) {
      case cnChooseThreshold_Both:
        negFalseCount--;
        negTrueCount++;
        break;
      case cnChooseThreshold_Far:
        negBothCount--;
        negTrueCount++;
        break;
      case cnChooseThreshold_Near:
        negBothCount++;
        negFalseCount--;
        break;
      }
    }
    // Calculate optimistic probabilities for both leaves. Assume both max.
    // TODO Is the highest prob really always best to make highest?
    // TODO Should I try fully both ways to see? Do math to prove?
    posAsTrueCount = posTrueCount + posBothCount;
    posAsFalseCount = posFalseCount + posBothCount;
    negAsTrueCount = negTrueCount + negBothCount;
    negAsFalseCount = negFalseCount + negBothCount;
    trueCount = posAsTrueCount + negAsTrueCount;
    falseCount = posAsFalseCount + negAsFalseCount;
    trueProb = posAsTrueCount / (cnFloat)trueCount;
    falseProb = posAsFalseCount / (cnFloat)falseCount;
    // Figure out which one really is the max.
    if (trueProb > falseProb) {
      // True wins the boths. Revert false.
      posAsFalseCount -= posBothCount;
      negAsFalseCount -= negBothCount;
      falseCount = posAsFalseCount + negAsFalseCount;
      falseProb = posFalseCount / (cnFloat)falseCount;
    } else {
      // False wins the boths. Revert true.
      posAsTrueCount -= posBothCount;
      negAsTrueCount -= negBothCount;
      trueCount = posAsTrueCount + negAsTrueCount;
      trueProb = posTrueCount / (cnFloat)trueCount;
    }
    // Calculate our log metric.
    metric = 0;
    if (posAsTrueCount) metric += posAsTrueCount * log(trueProb);
    if (negAsTrueCount) metric += negAsTrueCount * log(1 - trueProb);
    if (posAsFalseCount) metric += posAsFalseCount * log(falseProb);
    if (negAsFalseCount) metric += negAsFalseCount * log(1 - falseProb);
    /*printf(
      "Thresh: %.4lg (%.2lg of %ld, %.2lg of %ld: %.4lg)\n",
      cnChooseThreshold_EdgeDist(dist),
      trueProb, trueCount, falseProb, falseCount, metric
    );*/
    if (metric > bestMetric) {
      threshold = cnChooseThreshold_EdgeDist(dist);
      if (dist < distsEnd) {
        // Actually go halfway to the next, if there is one.
        threshold = (threshold + cnChooseThreshold_EdgeDist(dist + 1)) / 2;
      }
      bestMetric = metric;
      bestTrueProb = trueProb;
      bestFalseProb = falseProb;
      // # of max in each leaf.
      bestFalseCount = falseCount;
      bestTrueCount = trueCount;
    }
  }
  if (cnTrue) {
    printf(
      "Best thresh: %.4lg (%.2lg of %ld, %.2lg of %ld: %.4lg)\n",
      threshold, bestTrueProb, bestTrueCount, bestFalseProb, bestFalseCount,
      bestMetric
    );
  }

  // Free the pointer array, and return the threshold distance found.
  free(dists);
  return threshold;
}


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
  cnFloat* searchStart;
  cnBool result = cnFalse;
  cnList(cnPointBag) pointBags;

  cnListInit(&pointBags, sizeof(cnPointBag));
  if (!cnSplitNodePointBags(split, &pointBags)) {
    goto DISPOSE_VALUE_BAGS;
  }

  // It all worked.
  cnBuildInitialKernel(split->function->outTopology, &pointBags);
  searchStart =
    cnBestPointByDiverseDensity(split->function->outTopology, &pointBags);
  cnSearchFill(split->function->outTopology, &pointBags, searchStart);
  // TODO Learn a model already.
  result = cnTrue;

  // DROP_VALUE_BAGS:
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix);
  } cnEnd;

  DISPOSE_VALUE_BAGS:
  cnListDispose(&pointBags);
  return result;
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


void cnSearchFill(
  cnTopology topology, cnList(cnPointBag)* pointBags, cnFloat* searchStart
) {
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;
  cnBagDistance* distance;
  cnBagDistance* distances = malloc(pointBags->count * sizeof(cnBagDistance));
  cnBagDistance* distancesEnd = distances + pointBags->count;
  cnFloat* searchStartEnd = searchStart + valueCount;
  if (!distances) {
    // TODO Error result?
    return;
  }
  // Init distances to infinity.
  // TODO In iterative fill, these need to retain their former values.
  distance = distances;
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    distance->bag = pointBag;
    // Min is really 0, but -1 lets us see unchanged values.
    distance->far = -1;
    distance->near = HUGE_VAL;
    distance++;
  } cnEnd;
  // TODO Do I really want a search start?
  printf("Starting search at: ");
  cnVectorPrint(valueCount, searchStart);
  printf("\n");
  // Narrow the distance to each bag.
  for (distance = distances; distance < distancesEnd; distance++) {
    cnPointBag* pointBag = distance->bag;
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* pointsEnd = point + pointBag->pointCount * valueCount;
    if (!distance->near) continue; // Done with this one.
    // Look at each point in the bag.
    for (; point < pointsEnd; point += valueCount) {
      cnFloat currentDistance = 0;
      cnFloat* startValue;
      cnFloat* value;
      for (
        startValue = searchStart, value = point;
        startValue < searchStartEnd;
        startValue++, value++
      ) {
        // TODO Other distance metrics.
        cnFloat diff = *startValue - *value;
        currentDistance += diff * diff;
      }
      if (currentDistance > distance->far) {
        // New max found.
        // If currentDistance were NaN, the above > should fail, so we don't
        // expect to see any NaNs here.
        distance->far = currentDistance;
      }
      if (currentDistance < distance->near) {
        // New min found.
        // If currentDistance were NaN, the above < should fail, so we don't
        // expect to see any NaNs here.
        distance->near = currentDistance;
      }
    }
  }

  // Flip any unset fars also to infinity.
  for (distance = distances; distance < distancesEnd; distance++) {
    if (distance->far < 0) {
      distance->far = HUGE_VAL;
    }
  }

  // Find the right threshold for these distances and bag labels.
  cnChooseThreshold(distances, distancesEnd);

  // Clean up.
  free(distances);
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
