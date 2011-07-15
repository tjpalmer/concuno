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
 * Under the hood items needed for learning but which don't need exposed at the
 * API level.
 */
typedef struct cnLearnerConfig {

  /**
   * Need to keep track of the main info, too.
   */
  cnLearner* learner;

  cnRootNode* previous;

  cnList(cnBag) trainingBags;

  cnList(cnBag) validationBags;

} cnLearnerConfig;


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


cnRootNode* cnTryExpansionsAtLeaf(cnLearnerConfig* config, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
cnBool cnUpdateLeafProbabilities(cnRootNode* root);


/**
 * Performs a statistical test to verify that the candidate tree is a
 * significant improvement over the previous score.
 *
 * Returns true for non-error. The test result comes through the result param.
 */
cnBool cnVerifyImprovement(
  cnLearnerConfig* config, cnRootNode* candidate, cnBool* result
);


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
    cnFloat* point = pointBag->pointMatrix;
    cnFloat* matrixEnd = point + pointBag->pointCount * valueCount;
    if (posBagCount >= maxEitherBags && negBagCount >= maxEitherBags) break;
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

cnFloat cnChooseThreshold_edgeDist(const cnChooseThreshold_Distance* dist) {
  // The distances themselves. For both, we can use either.
  return dist->edge == cnChooseThreshold_Near ?
    dist->distance->near : dist->distance->far;
}

int cnChooseThreshold_compare(const void* a, const void* b) {
  cnFloat distA = cnChooseThreshold_edgeDist(a);
  cnFloat distB = cnChooseThreshold_edgeDist(b);
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
    sizeof(cnChooseThreshold_Distance), cnChooseThreshold_compare
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
        cnChooseThreshold_edgeDist(dist) == cnChooseThreshold_edgeDist(dist + 1)
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
    //      sqrt(cnChooseThreshold_edgeDist(dist)),
    //      yesProb, yesCount, noProb, noCount, currentScore
    //    );
    if (currentScore > bestScore) {
      threshold = cnChooseThreshold_edgeDist(dist);
      if (dist < distsEnd) {
        // Actually go halfway to the next, if there is one.
        threshold = (threshold + cnChooseThreshold_edgeDist(dist + 1)) / 2;
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
  cnLearnerInit(learner);
}


void cnLearnerInit(cnLearner* learner) {
  learner->bags = NULL;
  learner->entityFunctions = NULL;
  learner->initialTree = NULL;
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


cnRootNode* cnLearnTree(cnLearner* learner) {
  cnLearnerConfig config;
  cnRootNode* initialTree;
  cnLeafNode* leaf;
  cnList(cnLeafNode*) leaves;
  cnRootNode* result = NULL;

  // Start preparing the learning configuration.
  config.learner = learner;

  // Create a stub tree, if needed.
  initialTree = learner->initialTree;
  if (!initialTree) {
    // Allocate the root.
    initialTree = malloc(sizeof(cnRootNode));
    if (!initialTree) cnFailTo(DONE, "Failed to allocate root.");
    // Set up the tree.
    if (!cnRootNodeInit(initialTree, cnTrue)) {
      cnFailTo(DONE, "Failed to init stub tree.");
    }
  }

  // Split out training and validation sets.
  // TODO Uses 2/3 for training. Parameterize this?
  // Abuse lists to point into the middle of the original list.
  // Training set.
  cnListInit(&config.trainingBags, learner->bags->itemSize);
  config.trainingBags.items = learner->bags->items;
  config.trainingBags.count = learner->bags->count * (2 / 3.0);
  // Validation set.
  cnListInit(&config.validationBags, learner->bags->itemSize);
  config.validationBags.items =
    cnListGet(learner->bags, config.trainingBags.count);
  config.validationBags.count =
    learner->bags->count - config.trainingBags.count;
  // Failsafe on having data here.
  if (!config.validationBags.count) cnFailTo(DONE, "No validation bags!");

  // Propagate training bags to find initial probabilities.
  if (!cnRootNodePropagateBags(initialTree, &config.trainingBags)) {
    cnFailTo(DONE, "Failed to propagate bags in initial tree.");
  }
  // Make sure leaf probs are up to date.
  if (!cnUpdateLeafProbabilities(initialTree)) {
    cnFailTo(DONE, "Failed to update leaf probabilities.");
  }

  config.previous = initialTree;
  while (cnTrue) {
    cnRootNode* expanded;
    // And use our validation bags to see where we actually start.
    printf(
      "Initial metric: %lg\n",
      cnTreeLogMetric(config.previous, &config.validationBags)
    );

    /* Pick a leaf to expand. */ {
      // Get the leaves (over again, yes).
      cnListInit(&leaves, sizeof(cnLeafNode*));
      if (!cnNodeLeaves(&config.previous->node, &leaves)) {
        cnFailTo(DONE, "Failed to gather leaves.");
      }
      if (leaves.count < 1) {
        cnFailTo(DONE, "No leaves to expand.");
      }
      // TODO Pick the best leaf instead of the first.
      leaf = *(cnLeafNode**)leaves.items;
      // Clean up list of leaves.
      cnListDispose(&leaves);
    }

    expanded = cnTryExpansionsAtLeaf(&config, leaf);
    if (expanded) {
      if (result) {
        // Get rid of our old best.
        cnNodeDrop(&result->node);
      }
      result = expanded;
    } else {
      // Done expanding.
      break;
    }

    printf("Taking on another round!\n");
    config.previous = result;
  }
  // TODO If no most recent tree, return null or a clone as indicated in my
  // TODO other comments?
  printf("All done!!\n");

  DONE:
  if (!learner->initialTree) cnNodeDrop(&initialTree->node);
  // Don't actually dispose of training and validation lists, since they are
  // bogus anyway.
  return result;
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


typedef struct cnPushExpansionsByIndices_Data {
  cnList(cnExpansion)* expansions;
  cnExpansion* prototype;
  cnCount varDepth;
} cnPushExpansionsByIndices_Data;

cnBool cnPushExpansionsByIndices_Push(
  void* d, cnCount arity, cnIndex* indices
) {
  cnPushExpansionsByIndices_Data* data = d;
  cnIndex firstCommitted = data->varDepth - data->prototype->newVarCount;
  cnIndex v;

  // Make sure all our committed vars are there.
  // TODO Could instead compose only permutations with committed vars in them,
  // TODO but that's more complicated, and low arity makes the issue unlikely
  // TODO to matter for speed issues.
  for (v = firstCommitted; v < data->varDepth; v++) {
    cnBool found = cnFalse;
    cnIndex i;
    for (i = 0; i < arity; i++) {
      if (indices[i] == v) {
        found = cnTrue;
        break;
      }
    }
    if (!found) {
      // We're missing a committed var. Throw it back in the pond.
      return cnTrue;
    }
  }

  // Got our committed vars, but check against redundancy, too.
  if (cnExpansionRedundant(
    data->expansions, data->prototype->function, indices
  )) {
    // We already have an equivalent expansion under consideration.
    // TODO Is there a more efficient way to avoid redundancy?
    return cnTrue;
  }

  // It's good to go. Allocate space, and copy the indices.
  if (!(data->prototype->varIndices = malloc(arity * sizeof(cnIndex)))) {
    return cnFalse;
  }
  memcpy(data->prototype->varIndices, indices, arity * sizeof(cnIndex));

  // Push the expansion, copying the prototype.
  if (!cnListPush(data->expansions, data->prototype)) {
    free(data->prototype->varIndices);
    return cnFalse;
  }

  // Revert the prototype.
  data->prototype->varIndices = NULL;
  //printf("Done.\n");
  return cnTrue;
}

cnBool cnPushExpansionsByIndices(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth
) {
  cnPushExpansionsByIndices_Data data;
  data.expansions = expansions;
  data.prototype = prototype;
  data.varDepth = varDepth;
  return cnPermutations(
    varDepth, prototype->function->inCount,
    cnPushExpansionsByIndices_Push, &data
  );
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


cnRootNode* cnTryExpansionsAtLeaf(cnLearnerConfig* config, cnLeafNode* leaf) {
  cnFloat bestScore = -HUGE_VAL;
  cnRootNode* bestTree = NULL;
  cnList(cnEntityFunction)* entityFunctions = config->learner->entityFunctions;
  cnList(cnExpansion) expansions;
  cnCount maxArity = 0;
  cnCount minArity = LONG_MAX;
  cnCount minNewVarCount;
  cnCount newVarCount;
  cnCount varDepth = cnNodeVarDepth(&leaf->node);

  // Propagate training bags to get ready for training.
  if (
    !cnRootNodePropagateBags(cnNodeRoot(&leaf->node), &config->trainingBags)
  ) {
    cnFailTo(DONE, "Failed to propagate training bags.");
  }

  // Make a list of expansions. They can then be sorted, etc.
  cnListInit(&expansions, sizeof(cnExpansion));

  // Find the min and max arity.
  cnListEachBegin(entityFunctions, cnEntityFunction*, function) {
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
  varDepth += minNewVarCount;
  for (
    newVarCount = minNewVarCount;
    newVarCount <= maxArity;
    newVarCount++, varDepth++
  ) {
    cnExpansion expansion;

    cnListEachBegin(entityFunctions, cnEntityFunction*, function) {
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

    // TODO Error check expansions with just two leaves? Or always an error
    // TODO branch on var nodes? Is it better or worse to ask extra questions
    // TODO along the way?
  }

  // TODO Sort by arity? Or assume priority given by order? Some kind of
  // TODO heuristic?
  printf("Need to try %ld expansions.\n\n", expansions.count);
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    cnFloat score;

    cnRootNode* expanded = cnExpandedTree(config->learner, expansion);
    if (!expanded) goto DONE;

    // Check the metric on the validation set to see how we did.
    // TODO Evaluate LL to see if it's the best yet. If not ...
    // TODO Consider paired randomization test with validation set for
    // TODO significance test.
    score = cnTreeLogMetric(expanded, &config->validationBags);
    printf("Expanded tree has metric: %lg\n", score);
    if (score > bestScore) {
      // New best!
      printf(">>>-------->\n");
      printf(">>>--------> Best tree of this group!\n");
      printf(">>>-------->\n");
      // Out with the old, and in with the new.
      cnNodeDrop(&bestTree->node);
      bestScore = score;
      bestTree = expanded;
    } else {
      // No good. Dismiss it.
      // TODO Beam search?
      // TODO What if multiple bests with insignificant score difference?
      cnNodeDrop(&expanded->node);
    }

    // Blank line in this loop keeps expansions separated and easier to read.
    printf("\n");
  } cnEnd;

  // Significance test!
  if (bestTree) {
    cnBool result;
    if (!cnVerifyImprovement(config, bestTree, &result)) {
      cnFailTo(DONE, "Failed to accomplish verification.");
    }
    if (!result) {
      // The best wasn't good enough.
      cnNodeDrop(&bestTree->node);
      bestTree = NULL;
    }
  }

  DONE:
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    free(expansion->varIndices);
  } cnEnd;
  cnListDispose(&expansions);
  return bestTree;
}


cnBool cnUpdateLeafProbabilities(cnRootNode* root) {
  cnIndex b;
  cnCount bagCount;
  cnBag* bags;
  cnBool* bagsUsed = NULL;
  cnList(cnLeafNode*) leaves;
  cnBool result = cnFalse;

  // Get list of leaves.
  cnListInit(&leaves, sizeof(cnLeafNode*));
  if (!cnNodeLeaves(&root->node, &leaves)) goto DONE;

  // Init which bags used.
  // Assume the first bag at the root is the first bag in the original array.
  // TODO Organize to better clarify such expectations/constraints!
  bags = ((cnBindingBag*)root->node.bindingBagList->bindingBags.items)->bag;
  bagCount = root->node.bindingBagList->bindingBags.count;
  bagsUsed = malloc(bagCount * sizeof(cnBool));
  if (!bagsUsed) goto DONE;
  for (b = 0; b < bagCount; b++) bagsUsed[b] = cnFalse;

  // Loop through the leaves.
  while (leaves.count) {
    cnBindingBagList* bindingBags;
    cnLeafNode* maxLeaf = NULL;
    cnIndex maxLeafIndex = -1;
    cnFloat maxProb = -1;
    cnCount maxTotal = 0;

    // Count all remaining bags for each leaf.
    cnListEachBegin(&leaves, cnLeafNode*, leaf) {
      cnCount posCount = 0;
      cnCount total = 0;
      bindingBags = (*leaf)->node.bindingBagList;
      if (bindingBags) {
        cnListEachBegin(&bindingBags->bindingBags, cnBindingBag, bindingBag) {
          if (!bagsUsed[bindingBag->bag - bags]) {
            total++;
            posCount += bindingBag->bag->label;
          }
        } cnEnd;
      }
      // Assign optimistic probability, assuming 0 for no evidence.
      // TODO Without evidence, what's the best prior probability?
      (*leaf)->probability = total ? posCount / (cnFloat)total : 0;
      if ((*leaf)->probability > maxProb) {
        // Let the highest probability win.
        // TODO Let the highest individual score win? Compare the math on this.
        maxLeaf = *leaf;
        maxLeafIndex = leaf - (cnLeafNode**)leaves.items;
        maxProb = maxLeaf->probability;
        maxTotal = total;
      }
    } cnEnd;
    printf(
      "Leaf %ld of %ld with prob: %lf of %ld\n",
      maxLeafIndex + 1, leaves.count, maxLeaf->probability, maxTotal
    );

    // Remove the leaf, and mark its bags used.
    cnListRemove(&leaves, maxLeafIndex);
    bindingBags = maxLeaf->node.bindingBagList;
    if (bindingBags) {
      cnListEachBegin(&bindingBags->bindingBags, cnBindingBag, bindingBag) {
        bagsUsed[bindingBag->bag - bags] = cnTrue;
      } cnEnd;
    }
  }
  // We finished.
  result = cnTrue;

  DONE:
  free(bagsUsed);
  cnListDispose(&leaves);
  return result;
}


typedef struct cnVerifyImprovement_Stats {
  cnCount* bootCounts;
  cnList(cnLeafCount) leafCounts;
  cnFloat* probs;
} cnVerifyImprovement_Stats;

cnFloat cnVerifyImprovement_BootScore(
  cnVerifyImprovement_Stats* stats, cnCount count
) {
  // Generate the negative distribution.
  cnMultinomialSample(
    2 * stats->leafCounts.count, stats->bootCounts, count, stats->probs
  );

  // Overwrite the original leaf counts.
  cnListEachBegin(&stats->leafCounts, cnLeafCount, count) {
    cnIndex i = count - (cnLeafCount*)stats->leafCounts.items;
    // Neg first, then pos.
    count->negCount = stats->bootCounts[2 * i];
    count->posCount = stats->bootCounts[2 * i + 1];
  } cnEnd;

  // And get the metric from there.
  return cnCountsLogMetric(&stats->leafCounts);
}

void cnVerifyImprovement_StatsInit(cnVerifyImprovement_Stats* stats) {
  stats->bootCounts = NULL;
  cnListInit(&stats->leafCounts, sizeof(cnLeafCount));
  stats->probs = NULL;
}

void cnVerifyImprovement_StatsDispose(cnVerifyImprovement_Stats* stats) {
  free(stats->bootCounts);
  cnListDispose(&stats->leafCounts);
  free(stats->probs);
  // Clean out.
  cnVerifyImprovement_StatsInit(stats);
}

cnBool cnVerifyImprovement_StatsPrepare(
  cnVerifyImprovement_Stats* stats, cnRootNode* tree, cnList(cnBag)* bags
) {
  cnIndex i;
  cnBool result = cnFalse;

  // Gather up the original counts for each leaf.
  if (!cnTreeMaxLeafCounts(tree, &stats->leafCounts, bags)) {
    cnFailTo(DONE, "No counts.");
  }

  // Prepare place for stats.
  stats->probs = malloc(2 * stats->leafCounts.count * sizeof(cnFloat));
  stats->bootCounts = malloc(2 * stats->leafCounts.count * sizeof(cnCount));
  if (!(stats->probs && stats->bootCounts)) {
    cnFailTo(DONE, "No stats allocated.");
  }

  // Calculate probabilities.
  for (i = 0; i < stats->leafCounts.count; i++) {
    cnLeafCount* count = cnListGet(&stats->leafCounts, i);
    // Neg first, then pos.
    stats->probs[2 * i] = count->negCount / (cnFloat)bags->count;
    stats->probs[2 * i + 1] = count->posCount / (cnFloat)bags->count;
  }

  result = cnTrue;

  DONE:
  return result;
}

cnBool cnVerifyImprovement(
  cnLearnerConfig* config, cnRootNode* candidate, cnBool* result
) {

  // I don't know how to randomize across results from different trees.
  // Different probability assignments are possible.
  //
  // Instead, just bootstrap from each, and compare to see how often the
  // candidate wins. Note that for efficiency, we could keep around the
  // bootstrapped samples for the previous, but to simplify, just stick with
  // this for now.
  //
  // Also, for sampling efficiency here, instead of really sampling from the
  // validation set, just create multinomial distributions which should effect
  // the same result. We first need to decide how many positives and negatives
  // to use. Then we need to do multinomial sampling on distributions given by
  // the actual arrival of the validation bags at leaves.
  //
  // By working from a validation set, I avoid the need to correct for degrees
  // of freedom in the model. Hopefully in the end, this validation set with
  // bootstrap procedure is more accurate.
  //
  // TODO Make a general-purpose bootstrap procedure.

  // TODO Parameterize the boot repetition.
  cnCount bootRepeatCount = 10000;
  cnCount candidateWinCounts = 0;
  cnVerifyImprovement_Stats candidateStats;
  cnIndex i;
  cnBool okay = cnFalse;
  cnVerifyImprovement_Stats previousStats;
  cnFloat pValue;

  // Inits.
  cnVerifyImprovement_StatsInit(&candidateStats);
  cnVerifyImprovement_StatsInit(&previousStats);

  // Prepare stats for candidate and previous.
  printf("Candidate:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &candidateStats, candidate, &config->validationBags
  )) cnFailTo(DONE, "No stats.");
  printf("Previous:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &previousStats, config->previous, &config->validationBags
  )) cnFailTo(DONE, "No stats.");

  // Run the bootstrap.
  for (i = 0; i < bootRepeatCount; i++) {
    // Bootstrap each.
    cnFloat candidateScore = cnVerifyImprovement_BootScore(
      &candidateStats, config->validationBags.count
    );
    cnFloat previousScore = cnVerifyImprovement_BootScore(
      &previousStats, config->validationBags.count
    );
    candidateWinCounts += candidateScore > previousScore;
  }
  pValue = 1 - (candidateWinCounts / (cnFloat)bootRepeatCount);
  printf("Bootstrap p-value: %lg\n", pValue);
  // TODO Parameterize significance, especially in light of Bonferroni or Sidak
  // TODO correction.
  *result = pValue < 0.05;
  okay = cnTrue;

  DONE:
  // Cleanup is safe because of proper init.
  cnVerifyImprovement_StatsDispose(&candidateStats);
  cnVerifyImprovement_StatsDispose(&previousStats);

  return okay;
}

