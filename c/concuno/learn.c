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
 * Get the best point based on pseudo diverse density.
 *
 * TODO Provide more than one starting point? Or will KS flood fill be best
 * TODO for deciding what's too close or not?
 */
cnFloat* cnBestPointByDiverseDensity(
  cnTopology topology, cnList(cnPointBag)* pointBags
);


/**
 * Get the best point based on thresholding by the grand score.
 */
cnFloat* cnBestPointByScore(cnTopology topology, cnList(cnPointBag)* pointBags);


/**
 * Choose a threshold on the following distances to maximize the "noisy-and
 * noisy-or" metric, assuming that this determination is the only thing that
 * matters (no leaves elsewhere).
 *
 * If score is not null, assign the highest score to it. Also consider any
 * previously held values to be the highest previously. Therefore, if you
 * provide a pointer, make sure it's meaningful or else -HUGE_VAL.
 */
cnFloat cnChooseThreshold(
  cnBagDistance* distances, cnBagDistance* distancesEnd, cnFloat* score
);


/**
 * Returns a new tree with the expansion applied, optimized (so to speak)
 * according to the SMRF algorithm.
 *
 * TODO This might represent the best of multiple attempts at optimization.
 */
cnRootNode* cnExpandedTree(cnLearner* learner, cnExpansion* expansion);


cnBool cnExpansionRedundant(
  cnList(cnExpansion)* expansions, cnEntityFunction* function, cnIndex* indices
);


cnBool cnLearnSplitModel(cnLearner* learner, cnSplitNode* split);


void cnLogPointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags);


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
 * If a score is passed in, all scores less than it are ignored, and the final
 * value is changed to the best score found.
 *
 * On the other hand, threshold is only a return value, if the given pointer is
 * not null.
 *
 * TODO Provide a list of all contained positive points at end.
 *
 * TODO Actually, all I do here is find distances then pick a threshold.
 */
void cnSearchFill(
  cnTopology topology, cnList(cnPointBag)* pointBags, cnFloat* searchStart,
  cnFloat* score, cnFloat* threshold
);


cnRootNode* cnTryExpansionsAtLeaf(cnLearner* learner, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
cnBool cnUpdateLeafProbabilities(cnRootNode* root);


void cnBuildInitialKernel(cnTopology topology, cnList(cnPointBag)* pointBags) {
  cnFloat* positivePoint;
  cnFloat* positivePoints = NULL;
  cnCount positivePointCount;
  cnFloat* matrixEnd;
  cnCount valueCount = 0; // per point.
  cnFloat* point;

  if (topology != cnTopologyEuclidean) {
    printf("I handle only Euclidean right now, not %u.\n", topology);
    return;
  }

  // Count good, positive points.
  positivePointCount = 0;
  printf("pointBags->count: %ld\n", pointBags->count);
  if (pointBags->count > 0) {
    valueCount = ((cnPointBag*)pointBags->items)->valueCount;
  }
  printf("valueCount: %ld\n", valueCount);
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    point = pointBag->pointMatrix;
    matrixEnd = point + pointBag->pointCount * valueCount;
    for (; point < matrixEnd; point += valueCount) {
      cnBool allGood = cnTrue;
      cnFloat* value = point;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
      }
      positivePointCount += allGood;
    }
  } cnEnd;

  if (!positivePointCount) {
    printf("I need points to work with!\n");
    return;
  }

  // Make a matrix of the positive, good points.
  positivePoints = malloc(positivePointCount * valueCount * sizeof(cnFloat));
  if (!positivePoints) {
    goto DONE;
  }
  positivePoint = positivePoints;
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    point = pointBag->pointMatrix;
    matrixEnd = point + pointBag->pointCount * valueCount;
    for (; point < matrixEnd; point += valueCount) {
      cnBool allGood = cnTrue;
      cnFloat *value = point, *positiveValue = positivePoint;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++, positiveValue++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
        *positiveValue = *value;
      }
      if (allGood) {
        // Move to the next.
        positivePoint += valueCount;
      }
    }
  } cnEnd;

  printf(
    "Got a %ld by %ld matrix of positive points.\n",
    valueCount, positivePointCount
  );
  {
    // TODO Actually create a Gaussian PDF.
    cnFloat* stat = malloc(valueCount * sizeof(cnFloat));
    if (!stat) goto DONE;
    printf("Max, mean of positives: ");
    cnVectorPrint(stdout,
      valueCount,
      cnVectorMax(valueCount, stat, positivePointCount, positivePoints)
    );
    printf(", ");
    cnVectorPrint(stdout,
      valueCount,
      cnVectorMean(valueCount, stat, positivePointCount, positivePoints)
    );
    printf("\n");
    free(stat);
  }

  DONE:
  free(positivePoints);
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
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* matrixEnd = point + pointBag->pointCount * valueCount;
    if (!pointBag->bag->label) continue;
    if (posBagCount++ >= maxPosBags) break;
    printf("B ");
    for (; point < matrixEnd; point += valueCount) {
      cnFloat sumNegMin = 0, sumPosMin = 0;
      cnBool allGood = cnTrue;
      cnFloat* value;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
      }
      if (!allGood) continue;
      cnListEachBegin(pointBags, cnPointBag, pointBag2) {
        cnFloat minDistance = HUGE_VAL;
        cnFloat* point2 = pointBag2->pointMatrix;
        cnFloat* matrixEnd2 = point2 + pointBag2->pointCount * valueCount;
        for (; point2 < matrixEnd2; point2 += valueCount) {
          cnFloat distance = 0;
          cnFloat* value2;
          for (
            value = point, value2 = point2;
            value < pointEnd;
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
        cnVectorPrint(stdout, valueCount, point);
        printf(": %.4le) ", sumPosMin + sumNegMin);
        bestPoint = point;
        bestSumYet = sumPosMin + sumNegMin;
      }
    }
  } cnEnd;
  printf("\n");
  return bestPoint;
}


cnFloat* cnBestPointByScore(
  cnTopology topology, cnList(cnPointBag)* pointBags
) {
  cnFloat* bestPoint = NULL;
  cnFloat bestScore = -HUGE_VAL, score = bestScore;
  cnCount negBagCount = 0, posBagCount = 0, maxEitherBags = 8;
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;
  printf("Score-ish: ");
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    if (posBagCount >= maxEitherBags && negBagCount >= maxEitherBags) break;
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* matrixEnd = point + pointBag->pointCount * valueCount;
    if (pointBag->bag->label) {
      if (posBagCount++ >= maxEitherBags) continue;
    } else {
      if (negBagCount++ >= maxEitherBags) continue;
    }
    printf("B%c ", pointBag->bag->label ? '+' : '-');
    for (; point < matrixEnd; point += valueCount) {
      cnBool allGood = cnTrue;
      cnFloat* value;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = cnFalse;
          break;
        }
      }
      if (!allGood) continue;
      cnSearchFill(topology, pointBags, point, &score, NULL);
      // Print and check.
      if (score > bestScore) {
        printf("(");
        cnVectorPrint(stdout, valueCount, point);
        printf(": %.4le) ", score);
        bestPoint = point;
        bestScore = score;
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
  cnBagDistance* distances, cnBagDistance* distancesEnd, cnFloat* score
) {
  //  FILE* file = fopen("cnChooseThreshold.log", "w");
  // TODO Just allow sorting the original data instead of malloc here?
  cnCount bagCount = distancesEnd - distances;
  cnBagDistance* distance;
  cnChooseThreshold_Distance* dists =
    malloc(2 * bagCount * sizeof(cnChooseThreshold_Distance));
  cnChooseThreshold_Distance* dist;
  cnChooseThreshold_Distance* distsEnd = dists + 2 * bagCount;
  cnCount negBothCount = 0; // Negatives on both sides of the threshold.
  cnCount negNoCount = 0; // Negatives fully outside.
  cnCount negTotalCount = 0; // Total count of negatives.
  cnCount negYesCount = 0; // Negatives fully inside.
  cnCount posBothCount = 0; // Positives on both sides.
  cnCount posNoCount = 0; // Positives fully outside.
  cnCount posTotalCount = 0; // Total count of positives.
  cnCount posYesCount = 0; // Positives fully inside.
  cnCount negAsYesCount, negAsNoCount; // Neg count effectively in or out.
  cnCount posAsYesCount, posAsNoCount; // Pos count effectively in or out.
  cnCount yesCount, noCount; // Count of bags inside or outside.
  cnCount bestYesCount = 0, bestNoCount = 0;
  cnFloat bestScore, currentScore;
  cnFloat yesProb, bestYesProb = cnNaN();
  cnFloat noProb, bestNoProb = cnNaN();
  cnFloat threshold = 0;

  // Starting out assuming the worst. This actually log score, by the way.
  bestScore = score ? *score : -HUGE_VAL;
  //if (score) *score = bestScore;

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
      // TODO Is err probability relevant to the grand metric here? Here, it's
      // TODO just about bags with only err so far. We've ignored the case where
      // TODO only some bindings are err. They could impact, but just let those
      // TODO be considered as foreign leaves at some point, not kids. A big
      // TODO loop could prune those bags in later rounds if the err leaf picks
      // TODO them up.
      distsEnd -= 2;
      bagCount--;
    }
  }
  // Update the count now to the ones we care about.
  negTotalCount = bagCount - posTotalCount;
  // And they are all outside the threshold so far.
  posNoCount = posTotalCount;
  negNoCount = negTotalCount;
  //printf("Totals: %ld %ld (%d %ld)\n", posTotalCount, negTotalCount, distsEnd - dists, bagCount);

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
        posNoCount--;
        posYesCount++;
        break;
      case cnChooseThreshold_Far:
        posBothCount--;
        posYesCount++;
        break;
      case cnChooseThreshold_Near:
        posBothCount++;
        posNoCount--;
        break;
      }
    } else {
      // Change negative counts depending on leading, trailing, or both.
      switch (dist->edge) {
      case cnChooseThreshold_Both:
        negNoCount--;
        negYesCount++;
        break;
      case cnChooseThreshold_Far:
        negBothCount--;
        negYesCount++;
        break;
      case cnChooseThreshold_Near:
        negBothCount++;
        negNoCount--;
        break;
      }
    }
    // See if the next dist is the same as the current. This seems an odd case,
    // but I've seen it make a difference.
    if (dist + 1 < distsEnd) {
      if (
        cnChooseThreshold_EdgeDist(dist) == cnChooseThreshold_EdgeDist(dist + 1)
      ) {
        // They are equal, so they can't be separated. Move on.
        continue;
      }
    }
    // Calculate optimistic probabilities for both leaves. Assume both max.
    // TODO Is the highest prob really always best to make highest?
    // TODO Should I try fully both ways to see? Do math to prove?
    posAsYesCount = posYesCount + posBothCount;
    posAsNoCount = posNoCount + posBothCount;
    negAsYesCount = negYesCount + negBothCount;
    negAsNoCount = negNoCount + negBothCount;
    yesCount = posAsYesCount + negAsYesCount;
    noCount = posAsNoCount + negAsNoCount;
    yesProb = yesCount ? posAsYesCount / (cnFloat)yesCount : 0;
    noProb = noCount ? posAsNoCount / (cnFloat)noCount : 0;
    // Figure out which one really is the max.
    if (yesProb > noProb) {
      // Yes wins the boths. Revert no.
      posAsNoCount -= posBothCount;
      negAsNoCount -= negBothCount;
      noCount = posAsNoCount + negAsNoCount;
      noProb = noCount ? posAsNoCount / (cnFloat)noCount : 0;
    } else {
      // No wins the boths. Revert yes.
      posAsYesCount -= posBothCount;
      negAsYesCount -= negBothCount;
      yesCount = posAsYesCount + negAsYesCount;
      yesProb = yesCount ? posAsYesCount / (cnFloat)yesCount : 0;
    }
    // Calculate our log metric.
    currentScore = 0;
    if (posAsYesCount) currentScore += posAsYesCount * log(yesProb);
    if (negAsYesCount) currentScore += negAsYesCount * log(1 - yesProb);
    if (posAsNoCount) currentScore += posAsNoCount * log(noProb);
    if (negAsNoCount) currentScore += negAsNoCount * log(1 - noProb);
    //    fprintf(file,
    //      "%lg %lg %ld %lg %ld %lg\n",
    //      sqrt(cnChooseThreshold_EdgeDist(dist)),
    //      yesProb, yesCount, noProb, noCount, currentScore
    //    );
    if (currentScore > bestScore) {
      threshold = cnChooseThreshold_EdgeDist(dist);
      if (dist < distsEnd) {
        // Actually go halfway to the next, if there is one.
        threshold = (threshold + cnChooseThreshold_EdgeDist(dist + 1)) / 2;
      }
      bestScore = currentScore;
      bestYesProb = yesProb;
      bestNoProb = noProb;
      // # of max in each leaf.
      bestNoCount = noCount;
      bestYesCount = yesCount;
    }
  }
  if (cnTrue && !cnIsNaN(bestYesProb)) {
    printf(
      "Best thresh: %.9lg (%.2lg of %ld, %.2lg of %ld: %.4lg)\n",
      threshold, bestYesProb, bestYesCount, bestNoProb, bestNoCount,
      bestScore
    );
  }

  // Free the pointer array, provide the score if wanted, and return the thresh.
  free(dists);
  if (score) *score = bestScore;
  //  fclose(file);
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
  if (!cnUpdateLeafProbabilities(root)) {
    printf("Failed to update probabilities.\n");
    goto ERROR;
  }

  // Return the new tree.
  return root;

  ERROR:
  // Just destroy the whole tree copy for now in case of any error.
  cnNodeDrop(&root->node);
  return NULL;
}


cnBool cnExpansionRedundant(
  cnList(cnExpansion)* expansions, cnEntityFunction* function, cnIndex* indices
) {
  cnListEachBegin(expansions, cnExpansion, expansion) {
    if (function == expansion->function) {
      // TODO Mark which functions are symmetric. Assume all arity 2 for now.
      if (function->inCount == 2) {
        if (
          indices[0] == expansion->varIndices[1] &&
          indices[1] == expansion->varIndices[0]
        ) {
          // Same indices but reversed. This is redundant.
          // The function creates mirror images anyway.
          return cnTrue;
        }
      }
    }
  } cnEnd;
  // No duplicate found.
  return cnFalse;
}


void cnLearnerDispose(cnLearner* learner) {
  // Nothing yet.
}


void cnLearnerInit(cnLearner* learner) {
  // Nothing yet.
}


cnBool cnLearnSplitModel(cnLearner* learner, cnSplitNode* split) {
  // TODO Other topologies, etc.
  cnFunction* distanceFunction;
  cnGaussian* gaussian;
  cnFloat* searchStart;
  cnFloat threshold;
  cnBool result = cnFalse;
  cnList(cnPointBag) pointBags;

  cnListInit(&pointBags, sizeof(cnPointBag));
  if (!cnSplitNodePointBags(split, &pointBags)) {
    goto DISPOSE_POINT_BAGS;
  }
  //cnLogPointBags(split, &pointBags);

  // We got points. Try to learn something.
  // cnBuildInitialKernel(split->function->outTopology, &pointBags);
  searchStart =
    //cnBestPointByDiverseDensity(split->function->outTopology, &pointBags);
    cnBestPointByScore(split->function->outTopology, &pointBags);
  // TODO Determine better center and shape.
  // TODO Check for errors.
  cnSearchFill(
    split->function->outTopology, &pointBags, searchStart, NULL, &threshold
  );
  printf("Threshold: %lg\n", threshold);

  // We have an answer. Record it.
  // TODO Probably make the Gaussian earlier. It should be part of learning.
  // TODO
  // TODO Gaussian/Mahalanobis should be private to learning algorithm and
  // TODO specific to Euclidean topology. So probably have it built during
  // TODO learning. That is, learning could return a predicate.
  gaussian = malloc(sizeof(cnGaussian));
  if (!gaussian) {
    goto DROP_POINT_BAGS;
  }
  if (!cnGaussianInit(
    gaussian, split->function->outCount, searchStart
  )) {
    free(gaussian);
    goto DROP_POINT_BAGS;
  }
  distanceFunction = cnFunctionCreateMahalanobisDistance(gaussian);
  if (!distanceFunction) {
    cnGaussianDispose(gaussian);
    goto DROP_POINT_BAGS;
  }
  split->predicate = cnPredicateCreateDistanceThreshold(
    distanceFunction, threshold
  );
  if (!split->predicate) {
    cnFunctionDrop(distanceFunction);
    goto DROP_POINT_BAGS;
  }
  // Propagate for official leaf prob calculation.
  if (!cnNodePropagate(&split->node, NULL)) {
    // Leave the predicate around for later inspection and cleanup.
    goto DROP_POINT_BAGS;
  }
  // Good to go. Skip to the "do always" cleanup.
  result = cnTrue;
  goto DROP_POINT_BAGS;

  // Cleanup no matter what.
  DROP_POINT_BAGS:
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    free(pointBag->pointMatrix);
  } cnEnd;
  DISPOSE_POINT_BAGS:
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


void cnLogPointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags) {
  // TODO More filled out name. Easiest way to allocate as building?
  FILE *file;
  cnString name;

  // Prepare file name, and open/create output file.
  cnStringInit(&name);
  if (!cnListPushAll(&name, &split->function->name)) {
    cnStringDispose(&name);
    // TODO Error code.
    return;
  }
  // TODO Param indices.
  if (!cnStringPushStr(&name, ".log")) {
    cnStringDispose(&name);
    // TODO Error code.
    return;
  }
  file = fopen(cnStr(&name), "w");
  cnStringDispose(&name);
  if (!file) {
    return;
  }

  // Print out the data.
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* pointsEnd = point + pointBag->pointCount * pointBag->valueCount;
    for (; point < pointsEnd; point += pointBag->valueCount) {
      cnVectorPrint(file, pointBag->valueCount, point);
      fprintf(file, " %u", pointBag->bag->label);
      fprintf(file, "\n");
    }
  } cnEnd;

  // All done.
  fclose(file);
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
    // All filled up, but first check for redundancy.
    if (cnExpansionRedundant(expansions, prototype->function, indices)) {
      // We already have an equivalent expansion under consideration.
      // TODO Is there a more efficient way to avoid redundancy?
      return cnTrue;
    }
    // It's good to go.
    if (!(prototype->varIndices = malloc(arity * sizeof(cnIndex)))) {
      return cnFalse;
    }
    memcpy(prototype->varIndices, indices, arity * sizeof(cnIndex));
    // Keep the expansion.
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
  cnTopology topology, cnList(cnPointBag)* pointBags, cnFloat* searchStart,
  cnFloat* score, cnFloat* threshold
) {
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;
  cnBagDistance* distance;
  cnBagDistance* distances = malloc(pointBags->count * sizeof(cnBagDistance));
  cnBagDistance* distancesEnd = distances + pointBags->count;
  cnFloat* searchStartEnd = searchStart + valueCount;
  cnFloat thresholdStorage;
  if (!distances) {
    // TODO Error result?
    return;
  }
  // For convenience, point threshold at least somewhere.
  if (!threshold) threshold = &thresholdStorage;
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
  //  printf("Starting search at: ");
  //  cnVectorPrint(stdout, valueCount, searchStart);
  //  printf("\n");
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
        // TODO Defer to abstract distance function like cnMahalanobisDistance!
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

  // Flip any unset fars also to infinity, and sqrt the distances.
  for (distance = distances; distance < distancesEnd; distance++) {
    if (distance->far < 0) {
      distance->far = HUGE_VAL;
    } else {
      distance->far = sqrt(distance->far);
      distance->near = sqrt(distance->near);
    }
  }

  // Find the right threshold for these distances and bag labels.
  *threshold = cnChooseThreshold(distances, distancesEnd, score);

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
  cnListEachBegin(root->entityFunctions, cnEntityFunction*, function) {
    if ((*function)->inCount < minArity) {
      minArity = (*function)->inCount;
    }
    if ((*function)->inCount > maxArity) {
      maxArity = (*function)->inCount;
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
    cnListEachBegin(root->entityFunctions, cnEntityFunction*, function) {
      if ((*function)->inCount > varDepth) {
        //printf(
        //  "Need %ld more vars for %s.\n",
        //  function->inCount - varDepth, cnStr(&function->name)
        //);
        continue;
      }
      if ((*function)->inCount < newVarCount) {
        // We've already added more vars than we need for this one.
        //printf(
        //  "Added %ld too many vars for %s.\n",
        //  newVarCount - function->inCount, cnStr(&function->name)
        //);
        continue;
      }
      // Init a prototype expansion, then push index permutations.
      expansion.function = *function;
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
  // TODO Loop around grabbing the max each time, then prune binding bags.
  // TODO Perhaps loop through bags, assuming same order in all, to speed the
  // TODO match up.
  // TODO Faster might be to force the same sorting order on all, giving n log
  // TODO n, but where all comparison for later loops would be faster. Also
  // TODO makes fewer assumptions, but I think all the code elsewhere preserves
  // TODO order anyway.
  cnListInit(&leaves, sizeof(cnLeafNode*));
  if (!cnNodeLeaves(&root->node, &leaves)) {
    result = cnFalse;
    goto DONE;
  }
  printf("Found %ld leaves.\n", leaves.count);
  while (leaves.count) {
    cnLeafNode* maxLeaf = NULL;
    cnIndex maxLeafIndex = -1;
    cnFloat maxProb = -1;
    cnListEachBegin(&leaves, cnLeafNode*, leaf) {
      // TODO Find proper assignment for each leaf considering the others.
      cnCount posCount = 0;
      cnCount total = 0;
      cnBindingBagList* bindingBags = (*leaf)->node.bindingBagList;
      if (bindingBags) {
        total = bindingBags->bindingBags.count;
        cnListEachBegin(&bindingBags->bindingBags, cnBindingBag, bindingBag) {
          posCount += bindingBag->bag->label;
        } cnEnd;
      }
      // Assign optimistic probability, assuming 0 for no evidence.
      // TODO Without evidence, what's the best prior probability?
      (*leaf)->probability = total ? posCount / (cnFloat)total : 0;
      if ((*leaf)->probability > maxProb) {
        // Let the highest probability win.
        // TODO Let the highest individual score win? Compare the math on this.
        maxProb = (*leaf)->probability;
        maxLeaf = *leaf;
        maxLeafIndex = leaf - (cnLeafNode**)leaves.items;
      }
    } cnEnd;
    printf(
      "Leaf with prob: %lf of %ld\n",
      maxLeaf->probability,
      maxLeaf->node.bindingBagList ?
        maxLeaf->node.bindingBagList->bindingBags.count : 0
    );
    cnListRemove(&leaves, maxLeafIndex);
    /* Remove same bags from other leaves. */ {
      // Pointers for walking through bags.
      cnBindingBag* maxBindingBag = maxLeaf->node.bindingBagList ?
        maxLeaf->node.bindingBagList->bindingBags.items : NULL;
      cnBindingBag** otherBindingBags;
      cnBindingBag** otherBindingBag;
      cnBindingBag** otherBindingBagsEnd;
      // There aren't any bindings in the max? Well, move on.
      if (!maxBindingBag) continue;
      // Make space for the walking pointers.
      otherBindingBags = cnStackAlloc(leaves.count * sizeof(cnBindingBag*));
      if (!otherBindingBags) goto DONE;
      otherBindingBagsEnd = otherBindingBags + leaves.count;
      // Init the pointers for each leaf, so we can walk the lists in sync.
      for (
        otherBindingBag = otherBindingBags;
        otherBindingBag < otherBindingBagsEnd;
        otherBindingBag++
      ) {
        cnLeafNode* otherLeaf = *(cnLeafNode**)cnListGet(
          &leaves, otherBindingBag - otherBindingBags
        );
        *otherBindingBag = otherLeaf->node.bindingBagList ?
          otherLeaf->node.bindingBagList->bindingBags.items : NULL;
      }
      // Now sweep across all bags for all leaves in order.
      cnListEachBegin(
        &root->node.bindingBagList->bindingBags, cnBindingBag, bindingBag
      ) {
        cnBag* bag = bindingBag->bag;
        // First check for the bag in the max leaf.
        if (bag == maxBindingBag->bag) {
          // It's there. Remove it from others.
          for (
            otherBindingBag = otherBindingBags;
            otherBindingBag < otherBindingBagsEnd;
            otherBindingBag++
          ) {
            cnLeafNode* otherLeaf = *(cnLeafNode**)cnListGet(
              &leaves, otherBindingBag - otherBindingBags
            );
            if (
              *otherBindingBag &&
              *otherBindingBag < (cnBindingBag*)cnListEnd(
                &otherLeaf->node.bindingBagList->bindingBags
              ) &&
              bag == (*otherBindingBag)->bag
            ) {
              // It's a match. Dispose of the binding bag, then remove it.
              cnBindingBagDispose(*otherBindingBag);
              cnListRemove(
                &otherLeaf->node.bindingBagList->bindingBags,
                *otherBindingBag -
                  (cnBindingBag*)otherLeaf->node.bindingBagList->
                    bindingBags.items
              );
            }
          }
          // Advance the max-leaf bag counter.
          maxBindingBag++;
          // And see if we're done.
          if (
            maxBindingBag >= (cnBindingBag*)cnListEnd(
              &maxLeaf->node.bindingBagList->bindingBags
            )
          ) break;
        } else {
          // It's not in the max. Just advance past it in others.
          for (
            otherBindingBag = otherBindingBags;
            otherBindingBag < otherBindingBagsEnd;
            otherBindingBag++
          ) {
            cnLeafNode* otherLeaf = *(cnLeafNode**)cnListGet(
              &leaves, otherBindingBag - otherBindingBags
            );
            if (
              *otherBindingBag &&
              *otherBindingBag < (cnBindingBag*)cnListEnd(
                &otherLeaf->node.bindingBagList->bindingBags
              ) &&
              bag == (*otherBindingBag)->bag
            ) {
              // It's a match. Move past it.
              (*otherBindingBag)++;
            }
          }
        }
      } cnEnd;
      cnStackFree(otherBindingBags);
    }
  }
  // We finished.
  result = cnTrue;

  DONE:
  cnListDispose(&leaves);
  return result;
}
