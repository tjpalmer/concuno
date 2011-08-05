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
 * Get the best point based on thresholding by the grand score. If set to null,
 * no point could be found.
 *
 * Uses the distribution to manipulate the distance function. The distribution
 * is part of the result.
 *
 * TODO Rename and/or split this?
 *
 * TODO Maintain a distance function internally since that's driven from the
 * TODO distribution anyway?
 *
 * TODO Determine all leaf probabilities in determining threshold, or is that
 * TODO too expensive?
 *
 * TODO Always store the best point in the center when done? If so, use a
 * TODO different indicator for whether any point found?
 */
cnBool cnBestPointByScore(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags, cnFloat** bestPoint
);


/**
 * Choose the best threshold, centered on the point passed in.
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
cnBool cnChooseThreshold(
  cnFunction* distanceFunction, cnList(cnPointBag)* pointBags,
  cnFloat* score, cnFloat* threshold,
  cnList(cnFloat)* nearPosPoints, cnList(cnFloat)* nearNegPoints
);


/**
 * Choose a threshold on the following distances to maximize the "noisy-and
 * noisy-or" metric, assuming that this determination is the only thing that
 * matters (no leaves elsewhere).
 *
 * If score is not null, assign the highest score to it. Also consider any
 * previously held values to be the highest previously. Therefore, if you
 * provide a pointer, make sure it's meaningful or else -HUGE_VAL.
 */
cnFloat cnChooseThresholdWithDistances(
  cnBagDistance* distances, cnBagDistance* distancesEnd, cnFloat* score
);


/**
 * Returns a new tree with the expansion applied, optimized (so to speak)
 * according to the SMRF algorithm.
 *
 * TODO This might represent the best of multiple attempts at optimization.
 */
cnRootNode* cnExpandedTree(cnLearnerConfig* config, cnExpansion* expansion);


cnBool cnExpansionRedundant(
  cnList(cnExpansion)* expansions, cnEntityFunction* function, cnIndex* indices
);


cnBool cnLearnSplitModel(
  cnLearner* learner, cnSplitNode* split, cnList(cnBindingBag)* bindingBags
);


void cnLogPointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags);


/**
 * Finds a threshold, fits the distribution, and repeats (until what level of
 * convergence?).
 *
 * Need to abstract the distribution type.
 */
cnBool cnLoopFit(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags, cnFloat* threshold
);


cnLeafNode* cnPickBestLeaf(cnRootNode* tree, cnList(cnBag)* bags);


void cnPrintExpansion(cnExpansion* expansion);


cnBool cnPushExpansionsByIndices(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth
);


cnRootNode* cnTryExpansionsAtLeaf(cnLearnerConfig* config, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
cnBool cnUpdateLeafProbabilities(cnRootNode* root, cnList(cnBag)* bags);


/**
 * Updates leaf probabilities working directly from counts.
 */
cnBool cnUpdateLeafProbabilitiesWithBindingBags(
  cnList(cnLeafBindingBagGroup)* groups, cnList(cnLeafCount)* counts
);


/**
 * Performs a statistical test to verify that the candidate tree is a
 * significant improvement over the previous score.
 *
 * Returns true for non-error. The test result comes through the result param.
 */
cnBool cnVerifyImprovement(
  cnLearnerConfig* config, cnRootNode* candidate, cnFloat* pValue
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


cnBool cnBestPointByScore(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags, cnFloat** bestPoint
) {
  cnFloat bestScore = -HUGE_VAL, score = bestScore;
  cnCount negBagCount = 0, posBagCount = 0, maxEitherBags = 8;
  cnBool result = cnFalse;
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;

  // TODO Build a kd-tree (or set thereof) for approximate nearest neighbor
  // TODO sampling, and then use bounds to cut off searches once better scores
  // TODO seem impossible or at least unlikely.
  // TODO
  // TODO Is there also any way to estimate when there are too many points vs.
  // TODO bags to trust a situation?
  // TODO
  // TODO Is it worth watching wall-clock time to cut off a search? If I could
  // TODO interleave tree iterations, then this might be more reasonable. As in,
  // TODO some other expansion might already have lead to good trees and further
  // TODO expansions while some slow one is still chugging aways and not getting
  // TODO close to what's been going on elsewhere. Maybe some language like Go
  // TODO or Erlang or Stackless really would be nice. Hand designing the
  // TODO interleaving in C would take some effort, at least.
  // TODO
  // TODO Should I require positive bags to consider only cases with higher
  // TODO prob in yes branch, and for negative to have higher in no branch? That
  // TODO might help avoid overfit and also speed things up when considering
  // TODO bounds.
  // TODO
  // TODO In any case, the overfit matter might be moot with a validation set,
  // TODO so long as we can at least finish fast.

  // No best yet.
  *bestPoint = NULL;

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
    printf("B%c:%ld ", pointBag->bag->label ? '+' : '-', pointBag->pointCount);
    fflush(stdout);
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

      // Store the new center, then find the threshold.
      memcpy(distribution->mean, point, valueCount * sizeof(cnFloat));
      if (!
        cnChooseThreshold(distanceFunction, pointBags, &score, NULL, NULL, NULL)
      ) cnFailTo(DONE, "Search failed.");

      // Print and check.
      if (score > bestScore) {
        printf("(");
        cnVectorPrint(stdout, valueCount, point);
        printf(": %.4lg) ", score);
        *bestPoint = point;
        bestScore = score;
      }
      // Progress tracker.
      if (
        (1 + (point - (cnFloat*)pointBag->pointMatrix) / valueCount) % 100 == 0
      ) {
        printf(".");
        fflush(stdout);
      }
    }
  } cnEnd;
  printf("\n");

  // Winned!
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnChooseThreshold(
  cnFunction* distanceFunction, cnList(cnPointBag)* pointBags,
  cnFloat* score, cnFloat* threshold,
  cnList(cnFloat)* nearPosPoints, cnList(cnFloat)* nearNegPoints
) {
  cnCount valueCount =
    pointBags->count ? ((cnPointBag*)pointBags->items)->valueCount : 0;
  cnBagDistance* distance;
  cnBagDistance* distances = malloc(pointBags->count * sizeof(cnBagDistance));
  cnBagDistance* distancesEnd = distances + pointBags->count;
  cnBool result = cnFalse;
  cnFloat thresholdStorage;

  if (!distances) cnFailTo(DONE, "No distances.");

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
      // Find the distance and compare.
      cnFloat currentDistance;
      distanceFunction->evaluate(distanceFunction, point, &currentDistance);
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
  *threshold = cnChooseThresholdWithDistances(distances, distancesEnd, score);
  result = cnTrue;

  DONE:
  free(distances);
  return result;
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

cnFloat cnChooseThresholdWithDistances(
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


cnRootNode* cnExpandedTree(cnLearnerConfig* config, cnExpansion* expansion) {
  // TODO Loop across multiple inits/attempts?
  cnList(cnBindingBag)* bindingBags = NULL;
  cnLeafNode* leaf;
  cnList(cnLeafBindingBagGroup) leafBindingBagGroups;
  cnSplitNode* split;
  cnRootNode* root = NULL;
  cnCount varsAdded;
  printf("Expanding on "); cnPrintExpansion(expansion);

  // Init for safety.
  cnListInit(&leafBindingBagGroups, sizeof(cnLeafBindingBagGroup));

  // Create a copied tree to work with.
  if (!(
    root = (cnRootNode*)cnTreeCopy(&cnNodeRoot(&expansion->leaf->node)->node)
  )) cnFailTo(FAIL, "No tree copy for expansion.\n");
  // Find the new version of the leaf.
  leaf = (cnLeafNode*)cnNodeFindById(&root->node, expansion->leaf->node.id);

  // Add requested vars.
  for (varsAdded = 0; varsAdded < expansion->newVarCount; varsAdded++) {
    cnVarNode* var = cnVarNodeCreate(cnTrue);
    if (!var) cnFailTo(FAIL, "No var %ld for expansion.", varsAdded);
    cnNodeReplaceKid(&leaf->node, &var->node);
    leaf = *(cnLeafNode**)cnNodeKids(&var->node);
  }

  // Propagate training bags to get the bindings headed to our new leaf. We have
  // to do this after the var nodes, so we get the right bindings.
  // TODO Some way to specify that we only care about certain paths?
  if (!cnTreePropagateBags(
    root, &config->trainingBags, &leafBindingBagGroups
  )) cnFailTo(FAIL, "Failed to propagate training bags.");
  // Find the bindings we need from the to-be-replaced leaf.
  cnListEachBegin(&leafBindingBagGroups, cnLeafBindingBagGroup, group) {
    if (group->leaf == leaf) {
      bindingBags = &group->bindingBags;
      // TODO Could dispose of other groups, but we'd have to be tricky.
      break;
    }
  } cnEnd;

  // Add the split, and provide the bindings from the old parent.
  // TODO Just always propagate from the root?
  // TODO Don't bother even to store bindings??? How fast is it, usually?
  if (!(split = cnSplitNodeCreate(cnTrue))) cnFailTo(FAIL, "No split.");
  cnNodeReplaceKid(&leaf->node, &split->node);

  // Configure the split, and learn a model (distribution, threshold).
  split->function = expansion->function;
  // Could just borrow the indices, but that makes management more complicated.
  // TODO Array malloc/copy function?
  split->varIndices = malloc(split->function->inCount * sizeof(cnIndex));
  if (!split->varIndices) {
    cnFailTo(FAIL, "No var indices for expansion.");
  }
  memcpy(
    split->varIndices, expansion->varIndices,
    split->function->inCount * sizeof(cnIndex)
  );
  if (!cnLearnSplitModel(config->learner, split, bindingBags)) {
    cnFailTo(FAIL, "No split learned for expansion.");
  }
  // TODO Retain props from earlier?
  if (!cnUpdateLeafProbabilities(root, &config->trainingBags)) {
    cnFailTo(FAIL, "Failed to update probabilities.");
  }

  // Winned!
  goto DONE;

  FAIL:
  // Just destroy the whole tree copy for now in case of any error.
  cnNodeDrop(&root->node);
  root = NULL;

  DONE:
  cnLeafBindingBagGroupListDispose(&leafBindingBagGroups);
  return root;
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
  // TODO This will leak memory if someone nulls out the random beforehand!
  if (learner->randomOwned) cnRandomDestroy(learner->random);
  // Abusively pass down a potentially broken random.
  cnLearnerInit(learner, learner->random);
  // Clear out the random when done, either way.
  learner->random = NULL;
}


cnBool cnLearnerInit(cnLearner* learner, cnRandom random) {
  cnBool result = cnFalse;

  // Init for safety.
  learner->bags = NULL;
  learner->entityFunctions = NULL;
  learner->initialTree = NULL;
  learner->random = random;
  learner->randomOwned = cnFalse;

  // Prepare a random, if requested (via NULL).
  if (!random) {
    if (!(learner->random = cnRandomCreate())) {
      cnFailTo(DONE, "No default random.");
    }
    learner->randomOwned = cnTrue;
  }

  // Winned.
  result = cnTrue;

  DONE:
  return result;
}


cnBool cnLearnSplitModel(
  cnLearner* learner, cnSplitNode* split, cnList(cnBindingBag)* bindingBags
) {
  // TODO Other topologies, etc.
  cnFunction* distanceFunction;
  cnGaussian* gaussian;
  cnCount outCount = split->function->outCount;
  cnFloat* searchStart;
  cnFloat threshold = 0.0;
  cnBool result = cnFalse;
  cnList(cnPointBag) pointBags;

  // Get point bags.
  cnListInit(&pointBags, sizeof(cnPointBag));
  if (!cnSplitNodePointBags(split, bindingBags, &pointBags)) {
    goto DONE;
  }
  //cnLogPointBags(split, &pointBags);

  // Prepare a Gaussian for fitting to the data and a Mahalanobis distance
  // function based on that.
  //
  // TODO Abstract this more for support of other topologies and constraints.
  // TODO The discrete categories case will be especially different! No center!
  gaussian = malloc(sizeof(cnGaussian));
  if (!gaussian) {
    goto DONE;
  }
  if (!cnGaussianInit(gaussian, outCount, NULL)) {
    free(gaussian);
    goto DONE;
  }
  distanceFunction = cnFunctionCreateMahalanobisDistance(gaussian);
  if (!distanceFunction) {
    // The Gaussian can't be automatically cleaned if its container is defunct.
    cnGaussianDispose(gaussian);
    free(gaussian);
    goto DONE;
  }

  // We got points and a distance function. Try to learn something.
  // cnBuildInitialKernel(split->function->outTopology, &pointBags);
  // searchStart =
  //   cnBestPointByDiverseDensity(split->function->outTopology, &pointBags);
  if (!cnBestPointByScore(
    distanceFunction, gaussian, &pointBags, &searchStart
  )) cnFailTo(DONE, "Best point failure.");
  if (searchStart) {
    // Init the mean and kick the fit off.
    memcpy(gaussian->mean, searchStart, outCount * sizeof(cnFloat));
    if (!cnLoopFit(distanceFunction, gaussian, &pointBags, &threshold)) {
      cnFailTo(DONE, "Search failed.");
    }
  } else {
    // Nothing was any good. Any mean and threshold is arbitrary, so just let
    // them be 0. Mean was already defaulted to zero.
    threshold = 0.0;
  }
  printf("Threshold: %lg\n", threshold);

  // We have an answer. Record it.
  split->predicate = cnPredicateCreateDistanceThreshold(
    distanceFunction, threshold
  );
  if (!split->predicate) {
    // Cleans the Gaussian automatically.
    cnFunctionDrop(distanceFunction);
    goto DONE;
  }
  // Good to go. Skip to the "do always" cleanup.
  result = cnTrue;

  DONE:
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    cnPointBagDispose(pointBag);
  } cnEnd;
  cnListDispose(&pointBags);
  cnStackFree(center);
  return result;
}


cnRootNode* cnLearnTree(cnLearner* learner) {
  cnLearnerConfig config;
  cnRootNode* initialTree;
  cnLeafNode* leaf;
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

  // Make sure leaf probs are up to date.
  if (!cnUpdateLeafProbabilities(initialTree, &config.trainingBags)) {
    cnFailTo(DONE, "Failed to update leaf probabilities.");
  }

  config.previous = initialTree;
  while (cnTrue) {
    cnRootNode* expanded;
    // Print training score to observe conveniently the training progress.
    // TODO Could retain counts from the previous propagation to save the repeat
    // TODO here.
    printf(
      "Initial metric: %lg\n",
      cnTreeLogMetric(config.previous, &config.trainingBags)
    );

    // TODO Push all possible expansions for all leaves onto a heap.
    // TODO Pull the leaf binding bags in advance?
    if (!(leaf = cnPickBestLeaf(config.previous, &config.trainingBags))) {
      cnFailTo(DONE, "No leaf.");
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

    printf(
      "**********************************************************************\n"
      "**********************************************************************\n"
      "\n"
    );
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


cnBool cnLoopFit(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags, cnFloat* threshold
) {
  cnBool result = cnFalse;

  if (!cnChooseThreshold(
    // TODO Pass in lists for gathering affected points.
    distanceFunction, pointBags, NULL, threshold, NULL, NULL
  )) cnFailTo(DONE, "Search failed.");

  // TODO Determine better center and shape.

  result = cnTrue;

  DONE:
  return result;
}


cnLeafNode* cnPickBestLeaf(cnRootNode* tree, cnList(cnBag)* bags) {
  cnLeafNode* bestLeaf = NULL;
  cnLeafBindingBagGroup* bestGroup = NULL;
  cnFloat bestScore = -HUGE_VAL;
  cnList(cnLeafCount) counts;
  cnList(cnLeafNode) fakeLeaves;
  cnLeafNode* leaf;
  cnLeafBindingBagGroup* group;
  cnList(cnLeafBindingBagGroup) groups;
  cnList(cnList(cnIndex)) maxGroups;
  cnLeafBindingBagGroup* noGroup;
  cnLeafNode** realLeaves = NULL;

  // Init.
  cnListInit(&counts, sizeof(cnLeafCount));
  cnListInit(&fakeLeaves, sizeof(cnLeafNode));
  cnListInit(&groups, sizeof(cnLeafBindingBagGroup));
  cnListInit(&maxGroups, sizeof(cnList(cnIndex)));

  // Get all bindings, leaves.
  // TODO We don't actually care about the bindings themselves, but we had to
  // TODO make them to get past splits. Hrmm.
  if (!cnTreePropagateBags(tree, bags, &groups)) {
    cnFailTo(DONE, "No propagate.");
  }

  // Now find the binding bags which are max for each leaf. We'll use those for
  // our split, to avoid making low prob branches compete with high prob
  // branches that already selected the bags they like.
  if (!cnTreeMaxLeafBags(&groups, &maxGroups)) cnFailTo(DONE, "No max bags.");

  // Prepare for bogus "no" group. Error leaves with no bags are irrelevant.
  if (!(noGroup = cnListExpand(&groups))) {
    cnFailTo(DONE, "No space for bogus groups.");
  }
  // We shouldn't need a full real leaf to get by. Only the probability field
  // should end up used.
  cnListInit(&noGroup->bindingBags, sizeof(cnBindingBag));

  // Prepare bogus leaves for storing fake probs.
  // Also remember the real ones, so we can return the right leaf.
  if (!(
    realLeaves = malloc(groups.count * sizeof(cnLeafNode*))
  )) cnFailTo(DONE, "No real leaf memory.");
  if (!cnListExpandMulti(&fakeLeaves, groups.count)) {
    cnFailTo(DONE, "No space for bogus leaves.");
  }
  leaf = fakeLeaves.items;
  cnListEachBegin(&groups, cnLeafBindingBagGroup, group) {
    // Remember the original leaf.
    realLeaves[leaf - (cnLeafNode*)fakeLeaves.items] =
      group == noGroup ? NULL : group->leaf;
    // Overwrite the real leaf.
    group->leaf = leaf;
    leaf++;
  } cnEnd;

  // Look at each leaf to find the best perfect split. Loop on the max groups,
  // because they don't include the fake, and we don't need the fake.
  bestLeaf = NULL;
  group = groups.items;
  cnListEachBegin(&maxGroups, cnList(cnIndex), maxIndices) {
    // Split the leaf perfectly by removing all negative bags from the group and
    // putting them in the no group instead.
    cnIndex b;
    cnIndex bOriginal;
    cnBindingBag* bindingBag;
    cnIndex* maxIndex;
    cnIndex* maxIndicesEnd = cnListEnd(maxIndices);
    cnBindingBag* noEnd;
    cnFloat score;

    // Try to allocate maximal space in advance, so we don't have trouble later.
    // Just seems simpler (for cleanup) and not likely to be a memory issue.
    if (!cnListExpandMulti(&noGroup->bindingBags, group->bindingBags.count)) {
      cnFailTo(DONE, "No space for false bags.");
    }
    cnListClear(&noGroup->bindingBags);

    // Now do the split.
    bindingBag = group->bindingBags.items;
    maxIndex = maxIndices->items;
    for (b = 0, bOriginal = 0; b < group->bindingBags.count; bOriginal++) {
      cnBool isMax = maxIndex < maxIndicesEnd && bOriginal == *maxIndex;
      if (isMax) maxIndex++;
      if (isMax && bindingBag->bag->label) {
        // Positive maxes are the ones kept in the original leaf.
        b++;
        bindingBag++;
      } else {
        // Not a positive max, so move it.
        // The push is guaranteed to work, since we reserved space.
        cnListPush(&noGroup->bindingBags, bindingBag);
        // TODO Pointer-based remove function, then ditch b.
        cnListRemove(&group->bindingBags, b);
      }
    }
    printf("Max positives kept: %ld\n", group->bindingBags.count);
    printf("Others moved out: %ld\n", noGroup->bindingBags.count);

    // Update leaf probabilities, and find the score.
    if (
      !cnUpdateLeafProbabilitiesWithBindingBags(&groups, &counts)
    ) cnFailTo(DONE, "No fake leaf probs.");
    score = cnCountsLogMetric(&counts);
    printf(
      "Score at %ld: %lg",
      (cnIndex)(group - (cnLeafBindingBagGroup*)groups.items), score
    );
    if (score > bestScore) {
      bestGroup = group;
      bestScore = score;
      printf(" (best yet)");
    }
    printf("\n");

    // Then put them all back. Space is guaranteed again.
    bindingBag = noGroup->bindingBags.items;
    noEnd = cnListEnd(&noGroup->bindingBags);
    for (; bindingBag < noEnd; bindingBag++) {
      cnListPush(&group->bindingBags, bindingBag);
    }
    // And clear out the fake.
    cnListClear(&noGroup->bindingBags);

    // Advance the group.
    group++;
  } cnEnd;

  // Get the best leaf.
  if (bestGroup) {
    cnIndex bestIndex = bestGroup - (cnLeafBindingBagGroup*)groups.items;
    bestLeaf = realLeaves[bestIndex];
  }

  DONE:
  cnListEachBegin(&maxGroups, cnList(cnIndex), indices) {
    cnListDispose(indices);
  } cnEnd;
  cnListDispose(&maxGroups);
  cnLeafBindingBagGroupListDispose(&groups);
  cnListDispose(&fakeLeaves);
  cnListDispose(&counts);
  free(realLeaves);
  return bestLeaf;
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


cnRootNode* cnTryExpansionsAtLeaf(cnLearnerConfig* config, cnLeafNode* leaf) {
  cnFloat bestPValue = 1;
  cnRootNode* bestTree = NULL;
  cnList(cnEntityFunction)* entityFunctions = config->learner->entityFunctions;
  cnList(cnExpansion) expansions;
  cnCount maxArity = 0;
  cnCount minArity = LONG_MAX;
  cnCount minNewVarCount;
  cnCount newVarCount;
  cnCount varDepth = cnNodeVarDepth(&leaf->node);

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
    cnFloat pValue;
    cnRootNode* expanded;

    // Learn a tree.
    // TODO Disinguish bad errors from no good expansion?
    if (!(
      expanded = cnExpandedTree(config, expansion)
    )) cnFailTo(FAIL, "Expanding failed.");

    // Check the metric on the validation set to see how we did.
    // TODO Evaluate LL to see if it's the best yet. If not ...
    // TODO Consider paired randomization test with validation set for
    // TODO significance test.
    if (!cnVerifyImprovement(config, expanded, &pValue)) {
      cnNodeDrop(&expanded->node);
      cnFailTo(FAIL, "Failed propagate or p-value.");
    }
    printf("Expanded tree has p-value: %lg\n", pValue);
    if (pValue < bestPValue) {
      // New best!
      printf(">>>-------->\n");
      printf(">>>--------> Best tree of this group!\n");
      printf(">>>-------->\n");
      // Out with the old, and in with the new.
      cnNodeDrop(&bestTree->node);
      bestPValue = pValue;
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

  // Significance test.
  if (bestTree && bestPValue < 0.05) {
    // Good to go! TODO Multiple comparisons problem!!!
    goto DONE;
  }
  // The best wasn't good enough. Time to fail.

  FAIL:
  cnNodeDrop(&bestTree->node);
  bestTree = NULL;

  DONE:
  cnListEachBegin(&expansions, cnExpansion, expansion) {
    free(expansion->varIndices);
  } cnEnd;
  cnListDispose(&expansions);
  return bestTree;
}


cnBool cnUpdateLeafProbabilities(cnRootNode* root, cnList(cnBag)* bags) {
  cnList(cnLeafBindingBagGroup) groups;
  cnBool result = cnFalse;

  // Get all leaf binding bag groups.
  cnListInit(&groups, sizeof(cnLeafBindingBagGroup));
  if (!cnTreePropagateBags(root, bags, &groups)) {
    cnFailTo(DONE, "No propagate.");
  }

  // Update probs.
  if (!cnUpdateLeafProbabilitiesWithBindingBags(&groups, NULL)) {
    cnFailTo(DONE, "No probs.");
  }

  // Winned.
  result = cnTrue;

  DONE:
  cnLeafBindingBagGroupListDispose(&groups);
  return result;
}


cnBool cnUpdateLeafProbabilitiesWithBindingBags(
  cnList(cnLeafBindingBagGroup)* groups, cnList(cnLeafCount)* counts
) {
  cnIndex b;
  cnCount bagCount;
  cnBag* bags = NULL;
  cnBag* bagsEnd = NULL;
  cnBool* bagsUsed = NULL;
  cnFloat bonus = 1.0;
  cnList(cnLeafBindingBagGroup) groupsCopied;
  cnFloat previousProb = 1.0;
  cnBool result = cnFalse;

  // Copy this list, because we're going to be clearing leaf pointers.
  cnListInit(&groupsCopied, sizeof(cnLeafBindingBagGroup));
  if (!cnListPushAll(&groupsCopied, groups)) cnFailTo(DONE, "No groups copy.");

  // See if they want leaf counts, and get ready if they do.
  if (counts) {
    cnListClear(counts);
    if (!cnListExpandMulti(counts, groups->count)) cnFailTo(DONE, "No counts.");
  }

  // Init which bags used. First, we need to find how many and where they start.
  cnLeafBindingBagGroupListLimits(groups, &bags, &bagsEnd);
  bagCount = bagsEnd - bags;
  bagsUsed = malloc(bagCount * sizeof(cnBool));
  if (!bagsUsed) goto DONE;
  for (b = 0; b < bagCount; b++) bagsUsed[b] = cnFalse;

  // Loop through the groups.
  cnListEachBegin(&groupsCopied, cnLeafBindingBagGroup, group) {
    cnLeafBindingBagGroup* maxGroup = NULL;
    cnIndex maxGroupIndex = -1;
    cnCount maxPosCount = 0;
    cnFloat maxProb = -1;
    cnCount maxTotal = 0;

    // Count all remaining bags for each leaf.
    cnListEachBegin(&groupsCopied, cnLeafBindingBagGroup, group) {
      cnCount posCount = 0;
      cnCount total = 0;

      // See if we already finished this leaf.
      if (!group->leaf) continue;

      // Nope, so count bags.
      cnListEachBegin(&group->bindingBags, cnBindingBag, bindingBag) {
        if (!bagsUsed[bindingBag->bag - bags]) {
          total++;
          posCount += bindingBag->bag->label;
        }
      } cnEnd;

      // Assign optimistic probability, assuming 0.5 for no evidence.
      // Apply a beta prior with equal alpha and beta. I hate baked-in
      // parameters (or any parameters, really), but it's an easy fix for now.
      // On the other hand, beta of a=b=2 (for bonus of 1) is also known as the
      // rule of succession, introduced by Laplace. It's well founded.
      // TODO Still could parameterize the beta, if we want, but I hate
      // TODO parameters.
      group->leaf->probability = (total || bonus) ?
        (posCount + bonus) / (total + 2 * bonus) : 0.5;

      if (group->leaf->probability >= previousProb) {
        // This can happen in degenerate cases. Imagine leaves with the
        // following bag ids and labels:
        // Leaf 1: 1- 2- 3- 4- 5+ 6+ 7+ 8+ -> Prob 1/2
        // Leaf 2: 1- 2- 3- 9+             -> Prob 2/3 (or 1 without prior)
        // This is clearly wrong. It seems the score maximizing assignment is
        // 1/2 for both, but to keep clear assignments, let's separate by an
        // epsilon.
        // TODO Parameterize the epsilon or be more intelligent somehow?
        group->leaf->probability = (1.0 - 1e-6) * previousProb;
      }

      if (group->leaf->probability > maxProb) {
        // Let the highest probability win.
        // TODO Let the highest individual score win? Compare the math on this.
        maxGroup = group;
        maxGroupIndex = maxGroup - (cnLeafBindingBagGroup*)groupsCopied.items;
        maxPosCount = posCount;
        maxProb = maxGroup->leaf->probability;
        maxTotal = total;
      }
    } cnEnd;
    printf(
      "Leaf %ld with prob: %lf of %.2lf (really %lf of %ld)\n",
      maxGroupIndex + 1, maxProb, maxTotal + 2 * bonus,
      maxTotal ? maxPosCount / (cnFloat)maxTotal : 0.5, maxTotal
    );
    previousProb = maxProb;

    // Record the counts, if wanted.
    if (counts) {
      cnLeafCount* count = cnListGet(counts, maxGroupIndex);
      count->leaf = maxGroup->leaf;
      count->negCount = maxTotal - maxPosCount;
      count->posCount = maxPosCount;
    }

    // Mark the leaf and its bags as used.
    maxGroup->leaf = NULL;
    cnListEachBegin(&maxGroup->bindingBags, cnBindingBag, bindingBag) {
      bagsUsed[bindingBag->bag - bags] = cnTrue;
    } cnEnd;
  } cnEnd;

  // We finished.
  result = cnTrue;

  DONE:
  free(bagsUsed);
  cnListDispose(&groupsCopied);
  return result;
}


typedef struct cnVerifyImprovement_Stats {
  cnCount* bootCounts;
  cnList(cnLeafCount) leafCounts;
  cnMultinomial multinomial;
} cnVerifyImprovement_Stats;

cnFloat cnVerifyImprovement_BootScore(
  cnVerifyImprovement_Stats* stats, cnCount count
) {
  // Generate the leaf count distribution.
  cnMultinomialSample(stats->multinomial, stats->bootCounts);

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
  stats->multinomial = NULL;
}

void cnVerifyImprovement_StatsDispose(cnVerifyImprovement_Stats* stats) {
  free(stats->bootCounts);
  cnListDispose(&stats->leafCounts);
  cnMultinomialDestroy(stats->multinomial);
  // Clean out.
  cnVerifyImprovement_StatsInit(stats);
}

cnBool cnVerifyImprovement_StatsPrepare(
  cnVerifyImprovement_Stats* stats, cnRootNode* tree, cnList(cnBag)* bags,
  cnRandom random
) {
  cnCount classCount;
  cnIndex i;
  cnFloat* probs = NULL;
  cnBool result = cnFalse;

  // Gather up the original counts for each leaf.
  if (!cnTreeMaxLeafCounts(tree, &stats->leafCounts, bags)) {
    cnFailTo(DONE, "No counts.");
  }

  // Prepare place for stats.
  classCount = 2 * stats->leafCounts.count;
  probs = cnStackAlloc(classCount * sizeof(cnFloat));
  stats->bootCounts = malloc(classCount * sizeof(cnCount));
  if (!(probs && stats->bootCounts)) {
    cnFailTo(DONE, "No stats allocated.");
  }

  // Calculate probabilities.
  for (i = 0; i < stats->leafCounts.count; i++) {
    cnLeafCount* count = cnListGet(&stats->leafCounts, i);
    // Neg first, then pos.
    // TODO Apply the same beta prior as for updating probabilities? Otherwise,
    // TODO we might have inappropriate zeros (or even ones) in validation.
    // TODO Well, I guess it would be a Dirichlet prior, but it should be
    // TODO equivalent to the beta prior at the single leaf level.
    probs[2 * i] = count->negCount / (cnFloat)bags->count;
    probs[2 * i + 1] = count->posCount / (cnFloat)bags->count;
  }

  // Create the multinomial distribution.
  if (!(
    stats->multinomial =
      cnMultinomialCreate(random, bags->count, classCount, probs)
  )) cnFailTo(DONE, "No multinomial.")

  // Winned.
  result = cnTrue;

  DONE:
  cnStackFree(probs);
  return result;
}

cnBool cnVerifyImprovement(
  cnLearnerConfig* config, cnRootNode* candidate, cnFloat* pValue
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
  // TODO Make a general-purpose bootstrap procedure? Most of this would still
  // TODO be custom, I think.

  // TODO Gradually increase the boot repetition until we see convergence?
  cnCount bootRepeatCount = 10000;
  cnCount candidateWinCounts = 0;
  cnVerifyImprovement_Stats candidateStats;
  cnIndex i;
  cnBool okay = cnFalse;
  cnVerifyImprovement_Stats previousStats;

  // Inits.
  cnVerifyImprovement_StatsInit(&candidateStats);
  cnVerifyImprovement_StatsInit(&previousStats);

  // Prepare stats for candidate and previous.
  printf("Candidate:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &candidateStats, candidate, &config->validationBags, config->learner->random
  )) cnFailTo(DONE, "No stats.");
  printf("Previous:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &previousStats, config->previous, &config->validationBags,
    config->learner->random
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
  *pValue = 1 - (candidateWinCounts / (cnFloat)bootRepeatCount);
  okay = cnTrue;

  DONE:
  // Cleanup is safe because of proper init.
  cnVerifyImprovement_StatsDispose(&candidateStats);
  cnVerifyImprovement_StatsDispose(&previousStats);

  return okay;
}
