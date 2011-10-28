#include <limits.h>
#include <math.h>
#include <string.h>
#include "learn.h"
#include "mat.h"
#include "stats.h"


namespace concuno {


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

  /**
   * The actual point itself as stored in the point bag.
   */
  cnFloat* nearPoint;

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
  Learner* learner;

  cnRootNode* previous;

  cnList(Bag) trainingBags;

  cnList(Bag) validationBags;

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
bool cnBestPointByScore(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags,
  cnFunction** bestFunction, cnFloat* bestThreshold
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
bool cnChooseThreshold(
  bool yesLabel,
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
 *
 * Here, yesLabel says whether it expects the "yes" branch to have a higher
 * probability. We constrain this because it doesn't make sense to use positive
 * bags for learning negative features. It could happen legitimately, but it's
 * more likely about fishing for statistics.
 */
cnFloat cnChooseThresholdWithDistances(
  bool yesLabel,
  cnBagDistance* distances, cnBagDistance* distancesEnd, cnFloat* score,
  cnList(cnFloat)* nearPosPoints, cnList(cnFloat)* nearNegPoints
);


/**
 * Returns a new tree with the expansion applied, optimized (so to speak)
 * according to the SMRF algorithm.
 *
 * TODO This might represent the best of multiple attempts at optimization.
 */
cnRootNode* cnExpandedTree(cnLearnerConfig* config, cnExpansion* expansion);


bool cnExpansionRedundant(
  cnList(cnExpansion)* expansions, cnEntityFunction* function, cnIndex* indices
);


bool cnLearnSplitModel(
  Learner* learner, cnSplitNode* split, cnList(cnBindingBag)* bindingBags
);


void cnLogPointBags(cnSplitNode* split, cnList(cnPointBag)* pointBags);


cnLeafNode* cnPickBestLeaf(cnRootNode* tree, cnList(Bag)* bags);


void cnPrintExpansion(cnExpansion* expansion);


bool cnPushExpansionsByIndices(
  cnList(cnExpansion)* expansions, cnExpansion* prototype, cnCount varDepth
);


cnRootNode* cnTryExpansionsAtLeaf(cnLearnerConfig* config, cnLeafNode* leaf);


/**
 * Updates all the leaf probabilities in the tree.
 */
bool cnUpdateLeafProbabilities(cnRootNode* root, cnList(Bag)* bags);


/**
 * Updates leaf probabilities working directly from counts.
 */
bool cnUpdateLeafProbabilitiesWithBindingBags(
  cnList(cnLeafBindingBagGroup)* groups, cnList(cnLeafCount)* counts
);


/**
 * Performs a statistical test to verify that the candidate tree is a
 * significant improvement over the previous score.
 *
 * Returns true for non-error. The test result comes through the result param.
 */
bool cnVerifyImprovement(
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
    valueCount = ((cnPointBag*)pointBags->items)->pointMatrix.valueCount;
  }
  printf("valueCount: %ld\n", valueCount);
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    point = pointBag->pointMatrix.points;
    matrixEnd = point + pointBag->pointMatrix.pointCount * valueCount;
    for (; point < matrixEnd; point += valueCount) {
      bool allGood = true;
      cnFloat* value = point;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = false;
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
  positivePoints = cnAlloc(cnFloat, positivePointCount * valueCount);
  if (!positivePoints) {
    goto DONE;
  }
  positivePoint = positivePoints;
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    // Just use the positives for the initial kernel.
    if (!pointBag->bag->label) continue;
    point = pointBag->pointMatrix.points;
    matrixEnd = point + pointBag->pointMatrix.pointCount * valueCount;
    for (; point < matrixEnd; point += valueCount) {
      bool allGood = true;
      cnFloat *value = point, *positiveValue = positivePoint;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++, positiveValue++) {
        if (cnIsNaN(*value)) {
          allGood = false;
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
    cnFloat* stat = cnAlloc(cnFloat, valueCount);
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
  cnCount valueCount = pointBags->count ?
    ((cnPointBag*)pointBags->items)->pointMatrix.valueCount : 0;
  printf("DD-ish: ");
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    cnFloat* point = pointBag->pointMatrix.points;
    cnFloat* matrixEnd = point + pointBag->pointMatrix.pointCount * valueCount;
    if (!pointBag->bag->label) continue;
    if (posBagCount++ >= maxPosBags) break;
    printf("B ");
    for (; point < matrixEnd; point += valueCount) {
      cnFloat sumNegMin = 0, sumPosMin = 0;
      bool allGood = true;
      cnFloat* value;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = false;
          break;
        }
      }
      if (!allGood) continue;
      cnListEachBegin(pointBags, cnPointBag, pointBag2) {
        cnFloat minDistance = HUGE_VAL;
        cnFloat* point2 = pointBag2->pointMatrix.points;
        cnFloat* matrixEnd2 =
          point2 + pointBag2->pointMatrix.pointCount * valueCount;
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


bool cnBestPointByScore(
  cnFunction* distanceFunction, cnGaussian* distribution,
  cnList(cnPointBag)* pointBags,
  cnFunction** bestFunction, cnFloat* bestThreshold
) {
  cnFloat bestScore = -HUGE_VAL, score = bestScore;
  cnCount negBagsLeft = 8, posBagsLeft = 8;
  cnList(cnFloat) posPointsIn;
  bool result = false;
  cnFloat threshold;
  cnCount valueCount = pointBags->count ?
    ((cnPointBag*)pointBags->items)->pointMatrix.valueCount : 0;

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

  // Init point list.
  cnListInit(&posPointsIn, valueCount * sizeof(cnFloat));

  // No best yet.
  *bestFunction = NULL;
  *bestThreshold = cnNaN();

  printf("Score-ish: ");
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    cnFloat* point = pointBag->pointMatrix.points;
    cnFloat* matrixEnd = point + pointBag->pointMatrix.pointCount * valueCount;

    // See if we have already looked at enough bags.
    if (!(posBagsLeft || negBagsLeft)) break;
    if (pointBag->bag->label) {
      if (!posBagsLeft) continue;
      posBagsLeft--;
    } else {
      if (!negBagsLeft) continue;
      negBagsLeft--;
    }

    // Guess we're going to try this one out.
    printf(
      "B%c:%ld ",
      pointBag->bag->label ? '+' : '-', pointBag->pointMatrix.pointCount
    );
    fflush(stdout);
    for (; point < matrixEnd; point += valueCount) {
      bool allGood = true;
      cnFloat* value;
      cnFloat* pointEnd = point + valueCount;
      for (value = point; value < pointEnd; value++) {
        if (cnIsNaN(*value)) {
          allGood = false;
          break;
        }
      }
      if (!allGood) continue;

      // TODO Check all bests so far. We don't want too many. How to limit?
      // TODO Push everything onto the big heap? Probably not. Too much to
      // TODO track, and we'd need to see whether new points are in any of the
      // TODO many volumes under consideration. But I don't want an arbitrary
      // TODO threshold nor number, either. Threshold on potential p-value seems
      // TODO at least nicer than limiting quantity ...
      // TODO
      // TODO Checking if inside boundary requires looking at each, because
      // TODO would have a different threshold.
      // TODO
      // TODO With all points in all bags in a KD-Tree, we could easily remove
      // TODO from consideration all points scooped up to a certain point.
      if (*bestFunction) {
        // Check that we don't already contain this point.
        // This doesn't seem to speed things up much, perhaps due to the number
        // of inside points being small. However, it does allow to see where
        // we should split to separate options.
        // TODO This is dangerous until we can also climb means and fit!
        // TODO Check whether inside or outside is positive!!!
        cnFloat distance;
        if (!distanceFunction->evaluate(distanceFunction, point, &distance)) {
          cnErrTo(DONE, "No distance for point.");
        }
        // TODO Don't duplicate <= from the threshold predicate!
        if (distance <= threshold) goto SKIP_POINT;
      }

      // Store the new center, then find the threshold and contained points.
      memcpy(distribution->mean, point, valueCount * sizeof(cnFloat));
      cnListClear(&posPointsIn);
      if (!cnChooseThreshold(
        pointBag->bag->label,
        distanceFunction, pointBags, &score, &threshold, &posPointsIn, NULL
      )) cnErrTo(DONE, "Search failed.");

      // Check if best. TODO Check if better than any of the list of best.
      if (score > bestScore) {
        cnFloat fittedScore = -HUGE_VAL;

        // Fit the distribution to the contained points.
        // TODO Loop this fit, and check for convergence.
        // TODO Weighted? Negative points to push away?
        // TODO Abstract this to arbitrary fits.
        cnVectorMean(
          valueCount, distribution->mean, posPointsIn.count,
          reinterpret_cast<cnFloat*>(posPointsIn.items)
        );
        cnListClear(&posPointsIn);
        if (!cnChooseThreshold(
          pointBag->bag->label,
          distanceFunction, pointBags, &fittedScore, &threshold,
          &posPointsIn, NULL
        )) cnErrTo(DONE, "Search failed.");
        if (fittedScore < score) {
          // TODO This happens frequently, even for better end results. Why?
          printf("Fit worse (%.2lf < %.2lf)! ", fittedScore, score);
        }

        printf("(");
        cnVectorPrint(stdout, valueCount, distribution->mean);
        printf(": %.4lg) ", score);
        bestScore = score;

        // TODO Track multiple bests.
        cnFunctionDrop(*bestFunction);
        if (!(*bestFunction = cnFunctionCopy(distanceFunction))) {
          cnErrTo(DONE, "No best copy.");
        }
        *bestThreshold = threshold;
      }

      SKIP_POINT:
      // Progress tracker.
      if (
        (1 + (point - (cnFloat*)pointBag->pointMatrix.points) / valueCount)
        % 100 == 0
      ) {
        printf(".");
        fflush(stdout);
      }
    }
  } cnEnd;
  printf("\n");

  // Winned!
  result = true;

  DONE:
  cnListDispose(&posPointsIn);
  return result;
}


bool cnChooseThreshold(
  bool yesLabel,
  cnFunction* distanceFunction, cnList(cnPointBag)* pointBags,
  cnFloat* score, cnFloat* threshold,
  cnList(cnFloat)* nearPosPoints, cnList(cnFloat)* nearNegPoints
) {
  cnCount valueCount = pointBags->count ?
    ((cnPointBag*)pointBags->items)->pointMatrix.valueCount : 0;
  cnBagDistance* distance;
  cnBagDistance* distances = cnAlloc(cnBagDistance, pointBags->count);
  cnBagDistance* distancesEnd = distances + pointBags->count;
  bool result = false;
  cnFloat thresholdStorage;

  if (!distances) cnErrTo(DONE, "No distances.");

  // For convenience, point threshold at least somewhere.
  if (!threshold) threshold = &thresholdStorage;
  // Init distances to infinity.
  distance = distances;
  cnListEachBegin(pointBags, cnPointBag, pointBag) {
    distance->bag = pointBag;
    // Min is really 0, but -1 lets us see unchanged values.
    distance->far = -1;
    distance->near = HUGE_VAL;
    distance->nearPoint = NULL;
    distance++;
  } cnEnd;
  // TODO Do I really want a search start?
  //  printf("Starting search at: ");
  //  cnVectorPrint(stdout, valueCount, searchStart);
  //  printf("\n");
  // Narrow the distance to each bag.
  for (distance = distances; distance < distancesEnd; distance++) {
    cnPointBag* pointBag = distance->bag;
    cnFloat* point = pointBag->pointMatrix.points;
    cnFloat* pointsEnd = point + pointBag->pointMatrix.pointCount * valueCount;
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
        // Remember the near point for later, for when we want that.
        distance->nearPoint = point;
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
  *threshold = cnChooseThresholdWithDistances(
    yesLabel, distances, distancesEnd, score, nearPosPoints, nearNegPoints
  );
  result = true;

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
  cnFloat distA = cnChooseThreshold_edgeDist(
    reinterpret_cast<const cnChooseThreshold_Distance*>(a)
  );
  cnFloat distB = cnChooseThreshold_edgeDist(
    reinterpret_cast<const cnChooseThreshold_Distance*>(b)
  );
  return distA > distB ? 1 : distA == distB ? 0 : -1;
}

cnFloat cnChooseThresholdWithDistances(
  bool yesLabel,
  cnBagDistance* distances, cnBagDistance* distancesEnd, cnFloat* score,
  cnList(cnFloat)* nearPosPoints, cnList(cnFloat)* nearNegPoints
) {
  //  FILE* file = fopen("cnChooseThreshold.log", "w");
  // TODO Just allow sorting the original data instead of malloc here?
  cnCount bagCount = distancesEnd - distances;
  cnBagDistance* distance;
  cnChooseThreshold_Distance* dists =
    cnAlloc(cnChooseThreshold_Distance, 2 * bagCount);
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
      // Yes wins the boths. See if this is allowed.
      if (!yesLabel) continue;
      // It is. Revert no.
      posAsNoCount -= posBothCount;
      negAsNoCount -= negBothCount;
      noCount = posAsNoCount + negAsNoCount;
      noProb = noCount ? posAsNoCount / (cnFloat)noCount : 0;
    } else {
      // No wins the boths. See if this is allowed.
      if (yesLabel) continue;
      // It is. Revert yes.
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
      if (dist < distsEnd - 1) {
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
  if (true && !cnIsNaN(bestYesProb)) {
    printf(
      "Best thresh: %.9lg (%.2lg of %ld, %.2lg of %ld: %.4lg)\n",
      threshold, bestYesProb, bestYesCount, bestNoProb, bestNoCount,
      bestScore
    );
  }

  // See if points are wanted, and provide those if so.
  if (nearPosPoints || nearNegPoints) {
    for (dist = dists; dist < distsEnd; dist++) {
      // Both near and both are good enough.
      if (dist->edge != cnChooseThreshold_Far) {
        cnList(cnFloat)* nearPoints;
        // See if we've passed the insides.
        // TODO Remove redundancy of <= (or '! <=' as '>' here)!!!
        if (dist->distance->near > threshold) break;
        // It's a near distance, so that's what we want. See if pos or neg.
        // TODO Rename pos and neg for matching and non-matching??? The idea is
        // TODO which points contribute to the higher vs. lower probability.
        nearPoints = dist->distance->bag->bag->label ?
          (yesLabel ? nearPosPoints : nearNegPoints) :
          (yesLabel ? nearNegPoints : nearPosPoints);
        // Add the point, if we want this kind.
        if (nearPoints) {
          // Condensing these to an independent matrix. Costly? Probably not
          // just for those in the threshold? Or in ugly cases, could it get
          // bad?
          cnListPush(nearPoints, dist->distance->nearPoint);
        }
      }
    }
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
  )) cnErrTo(FAIL, "No tree copy for expansion.\n");
  // Find the new version of the leaf.
  leaf = (cnLeafNode*)cnNodeFindById(&root->node, expansion->leaf->node.id);

  // Add requested vars.
  for (varsAdded = 0; varsAdded < expansion->newVarCount; varsAdded++) {
    cnVarNode* var = cnVarNodeCreate(true);
    if (!var) cnErrTo(FAIL, "No var %ld for expansion.", varsAdded);
    cnNodeReplaceKid(&leaf->node, &var->node);
    leaf = *(cnLeafNode**)cnNodeKids(&var->node);
  }

  // Propagate training bags to get the bindings headed to our new leaf. We have
  // to do this after the var nodes, so we get the right bindings.
  // TODO Some way to specify that we only care about certain paths?
  if (!cnTreePropagateBags(
    root, &config->trainingBags, &leafBindingBagGroups
  )) cnErrTo(FAIL, "Failed to propagate training bags.");
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
  if (!(split = cnSplitNodeCreate(true))) cnErrTo(FAIL, "No split.");
  cnNodeReplaceKid(&leaf->node, &split->node);

  // Configure the split, and learn a model (distribution, threshold).
  split->function = expansion->function;
  // Could just borrow the indices, but that makes management more complicated.
  // TODO Array malloc/copy function?
  split->varIndices = cnAlloc(cnIndex, split->function->inCount);
  if (!split->varIndices) {
    cnErrTo(FAIL, "No var indices for expansion.");
  }
  memcpy(
    split->varIndices, expansion->varIndices,
    split->function->inCount * sizeof(cnIndex)
  );
  if (!cnLearnSplitModel(config->learner, split, bindingBags)) {
    cnErrTo(FAIL, "No split learned for expansion.");
  }
  // TODO Retain props from earlier?
  if (!cnUpdateLeafProbabilities(root, &config->trainingBags)) {
    cnErrTo(FAIL, "Failed to update probabilities.");
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


bool cnExpansionRedundant(
  cnList(cnExpansion)* expansions, cnEntityFunction* function, cnIndex* indices
) {
  // Hack assuming all things are symmetric, and we only want increasing order.
  // TODO Formalize this. Delegate to the functions themselves!
  cnIndex i;
  for (i = 1; i < function->inCount; i++) {
    if (indices[i] <= indices[i - 1]) {
      return true;
    }
  }

  /*// Mode assuming only certain parameters are symmetric.
  cnListEachBegin(expansions, cnExpansion, expansion) {
    if (function == expansion->function) {
      // TODO Mark which functions are symmetric. Assume all arity 2 for now.
      // TODO Actually, for reframe with arity 3, the first two are still
      // TODO symmetric. That hack is here for now, but it should be with the
      // TODO functions.
      if (function->inCount == 2 || function->inCount == 3) {
        if (
          indices[0] == expansion->varIndices[1] &&
          indices[1] == expansion->varIndices[0]
        ) {
          // Same indices but reversed. This is redundant.
          // The function creates mirror images anyway.
          return
            function->inCount == 2 || indices[2] == expansion->varIndices[2];
        }
      }
    }
  } cnEnd;
  */

  // No duplicate found.
  return false;
}


void cnLearnerDispose(Learner* learner) {
  // TODO This will leak memory if someone nulls out the random beforehand!
  if (learner->randomOwned) cnRandomDestroy(learner->random);
  // Abusively pass down a potentially broken random.
  cnLearnerInit(learner, learner->random);
  // Clear out the random when done, either way.
  learner->random = NULL;
}


bool cnLearnerInit(Learner* learner, cnRandom random) {
  bool result = false;

  // Init for safety.
  learner->bags = NULL;
  learner->entityFunctions = NULL;
  learner->initialTree = NULL;
  learner->random = random;
  learner->randomOwned = false;

  // Prepare a random, if requested (via NULL).
  if (!random) {
    if (!(learner->random = cnRandomCreate())) {
      cnErrTo(DONE, "No default random.");
    }
    learner->randomOwned = true;
  }

  // Winned.
  result = true;

  DONE:
  return result;
}


bool cnLearnSplitModel(
  Learner* learner, cnSplitNode* split, cnList(cnBindingBag)* bindingBags
) {
  // TODO Other topologies, etc.
  cnFunction* bestFunction = NULL;
  cnFunction* distanceFunction;
  cnGaussian* gaussian;
  cnCount outCount = split->function->outCount;
  cnFloat threshold = 0.0;
  bool result = false;
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
  gaussian = cnAlloc(cnGaussian, 1);
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
    distanceFunction, gaussian, &pointBags, &bestFunction, &threshold
  )) cnErrTo(DONE, "Best point failure.");
  if (bestFunction) {
    // Replace the initial with the best found.
    cnFunctionDrop(distanceFunction);
    distanceFunction = bestFunction;
  } else {
    // Nothing was any good. Any mean and threshold is arbitrary, so just let
    // them be 0. Mean was already defaulted to zero.
    threshold = 0.0;
  }

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
  result = true;

  DONE:
  cnListEachBegin(&pointBags, cnPointBag, pointBag) {
    cnPointBagDispose(pointBag);
  } cnEnd;
  cnListDispose(&pointBags);
  cnStackFree(center);
  return result;
}


cnRootNode* cnLearnTree(Learner* learner) {
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
    initialTree = cnAlloc(cnRootNode, 1);
    if (!initialTree) cnErrTo(DONE, "Failed to allocate root.");
    // Set up the tree.
    if (!cnRootNodeInit(initialTree, true)) {
      cnErrTo(DONE, "Failed to init stub tree.");
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
  if (!config.validationBags.count) cnErrTo(DONE, "No validation bags!");

  // Make sure leaf probs are up to date.
  if (!cnUpdateLeafProbabilities(initialTree, &config.trainingBags)) {
    cnErrTo(DONE, "Failed to update leaf probabilities.");
  }

  config.previous = initialTree;
  while (true) {
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
      cnErrTo(DONE, "No leaf.");
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
    cnFloat* point = pointBag->pointMatrix.points;
    cnFloat* pointsEnd =
      point +
      pointBag->pointMatrix.pointCount * pointBag->pointMatrix.valueCount;
    for (; point < pointsEnd; point += pointBag->pointMatrix.valueCount) {
      cnVectorPrint(file, pointBag->pointMatrix.valueCount, point);
      fprintf(file, " %u", pointBag->bag->label);
      fprintf(file, "\n");
    }
  } cnEnd;

  // All done.
  fclose(file);
}


cnLeafNode* cnPickBestLeaf(cnRootNode* tree, cnList(Bag)* bags) {
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
    cnErrTo(DONE, "No propagate.");
  }

  // Now find the binding bags which are max for each leaf. We'll use those for
  // our split, to avoid making low prob branches compete with high prob
  // branches that already selected the bags they like.
  if (!cnTreeMaxLeafBags(&groups, &maxGroups)) cnErrTo(DONE, "No max bags.");

  // Prepare for bogus "no" group. Error leaves with no bags are irrelevant.
  if (!(
    noGroup = reinterpret_cast<cnLeafBindingBagGroup*>(cnListExpand(&groups))
  )) {
    cnErrTo(DONE, "No space for bogus groups.");
  }
  // We shouldn't need a full real leaf to get by. Only the probability field
  // should end up used.
  cnListInit(&noGroup->bindingBags, sizeof(cnBindingBag));

  // Prepare bogus leaves for storing fake probs.
  // Also remember the real ones, so we can return the right leaf.
  if (!(realLeaves = cnAlloc(cnLeafNode*, groups.count))) {
    cnErrTo(DONE, "No real leaf memory.");
  }
  if (!cnListExpandMulti(&fakeLeaves, groups.count)) {
    cnErrTo(DONE, "No space for bogus leaves.");
  }
  leaf = reinterpret_cast<cnLeafNode*>(fakeLeaves.items);
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
  group = reinterpret_cast<cnLeafBindingBagGroup*>(groups.items);
  cnListEachBegin(&maxGroups, cnList(cnIndex), maxIndices) {
    // Split the leaf perfectly by removing all negative bags from the group and
    // putting them in the no group instead.
    cnIndex b;
    cnIndex bOriginal;
    cnBindingBag* bindingBag;
    cnIndex* maxIndex;
    cnIndex* maxIndicesEnd = reinterpret_cast<cnIndex*>(cnListEnd(maxIndices));
    cnBindingBag* noEnd;
    cnFloat score;

    // If this group is empty, just skip it.
    if (!group->bindingBags.count) continue;

    // Try to allocate maximal space in advance, so we don't have trouble later.
    // Just seems simpler (for cleanup) and not likely to be a memory issue.
    if (!cnListExpandMulti(&noGroup->bindingBags, group->bindingBags.count)) {
      cnErrTo(DONE, "No space for false bags.");
    }
    cnListClear(&noGroup->bindingBags);

    // Now do the split.
    bindingBag = reinterpret_cast<cnBindingBag*>(group->bindingBags.items);
    maxIndex = reinterpret_cast<cnIndex*>(maxIndices->items);
    for (b = 0, bOriginal = 0; b < group->bindingBags.count; bOriginal++) {
      bool isMax = maxIndex < maxIndicesEnd && bOriginal == *maxIndex;
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
    ) cnErrTo(DONE, "No fake leaf probs.");
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
    bindingBag = reinterpret_cast<cnBindingBag*>(noGroup->bindingBags.items);
    noEnd = reinterpret_cast<cnBindingBag*>(cnListEnd(&noGroup->bindingBags));
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

bool cnPushExpansionsByIndices_Push(
  void* d, cnCount arity, cnIndex* indices
) {
  cnPushExpansionsByIndices_Data* data =
    reinterpret_cast<cnPushExpansionsByIndices_Data*>(d);
  cnIndex firstCommitted = data->varDepth - data->prototype->newVarCount;
  cnIndex v;

  // Make sure all our committed vars are there.
  // TODO Could instead compose only permutations with committed vars in them,
  // TODO but that's more complicated, and low arity makes the issue unlikely
  // TODO to matter for speed issues.
  for (v = firstCommitted; v < data->varDepth; v++) {
    bool found = false;
    cnIndex i;
    for (i = 0; i < arity; i++) {
      if (indices[i] == v) {
        found = true;
        break;
      }
    }
    if (!found) {
      // We're missing a committed var. Throw it back in the pond.
      return true;
    }
  }

  // Got our committed vars, but check against redundancy, too.
  if (cnExpansionRedundant(
    data->expansions, data->prototype->function, indices
  )) {
    // We already have an equivalent expansion under consideration.
    // TODO Is there a more efficient way to avoid redundancy?
    return true;
  }

  // It's good to go. Allocate space, and copy the indices.
  if (!(data->prototype->varIndices = cnAlloc(cnIndex, arity))) {
    return false;
  }
  memcpy(data->prototype->varIndices, indices, arity * sizeof(cnIndex));

  // Push the expansion, copying the prototype.
  if (!cnListPush(data->expansions, data->prototype)) {
    free(data->prototype->varIndices);
    return false;
  }

  // Revert the prototype.
  data->prototype->varIndices = NULL;
  //printf("Done.\n");
  return true;
}

bool cnPushExpansionsByIndices(
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
    )) cnErrTo(FAIL, "Expanding failed.");

    // Check the metric on the validation set to see how we did.
    // TODO Evaluate LL to see if it's the best yet. If not ...
    // TODO Consider paired randomization test with validation set for
    // TODO significance test.
    if (!cnVerifyImprovement(config, expanded, &pValue)) {
      cnNodeDrop(&expanded->node);
      cnErrTo(FAIL, "Failed propagate or p-value.");
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
  if (bestTree && bestPValue < 0.1) {
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


bool cnUpdateLeafProbabilities(cnRootNode* root, cnList(Bag)* bags) {
  cnList(cnLeafBindingBagGroup) groups;
  bool result = false;

  // Get all leaf binding bag groups.
  cnListInit(&groups, sizeof(cnLeafBindingBagGroup));
  if (!cnTreePropagateBags(root, bags, &groups)) {
    cnErrTo(DONE, "No propagate.");
  }

  // Update probs.
  if (!cnUpdateLeafProbabilitiesWithBindingBags(&groups, NULL)) {
    cnErrTo(DONE, "No probs.");
  }

  // Winned.
  result = true;

  DONE:
  cnLeafBindingBagGroupListDispose(&groups);
  return result;
}


bool cnUpdateLeafProbabilitiesWithBindingBags(
  cnList(cnLeafBindingBagGroup)* groups, cnList(cnLeafCount)* counts
) {
  cnIndex b;
  cnCount bagCount;
  Bag* bags = NULL;
  Bag* bagsEnd = NULL;
  bool* bagsUsed = NULL;
  cnFloat bonus = 1.0;
  cnList(cnLeafBindingBagGroup) groupsCopied;
  cnFloat previousProb = 1.0;
  bool result = false;

  // Copy this list, because we're going to be clearing leaf pointers.
  cnListInit(&groupsCopied, sizeof(cnLeafBindingBagGroup));
  if (!cnListPushAll(&groupsCopied, groups)) cnErrTo(DONE, "No groups copy.");

  // See if they want leaf counts, and get ready if they do.
  if (counts) {
    cnListClear(counts);
    if (!cnListExpandMulti(counts, groups->count)) cnErrTo(DONE, "No counts.");
  }

  // Init which bags used. First, we need to find how many and where they start.
  cnLeafBindingBagGroupListLimits(groups, &bags, &bagsEnd);
  bagCount = bagsEnd - bags;
  bagsUsed = cnAlloc(bool, bagCount);
  if (!bagsUsed) goto DONE;
  for (b = 0; b < bagCount; b++) bagsUsed[b] = false;

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
      group->leaf->strength = total + 2 * bonus;
      group->leaf->probability = (total || bonus) ?
        (posCount + bonus) / group->leaf->strength : 0.5;

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
      maxGroupIndex + 1, maxProb, maxGroup->leaf->strength,
      maxTotal ? maxPosCount / (cnFloat)maxTotal : 0.5, maxTotal
    );
    previousProb = maxProb;

    // Record the counts, if wanted.
    if (counts) {
      cnLeafCount* count =
        reinterpret_cast<cnLeafCount*>(cnListGet(counts, maxGroupIndex));
      count->leaf = maxGroup->leaf;
      count->negCount = maxTotal - maxPosCount;
      count->posCount = maxPosCount;
    }

    // Mark the leaf and its bags as used.
    maxGroup->leaf = NULL;
    cnListEachBegin(&maxGroup->bindingBags, cnBindingBag, bindingBag) {
      bagsUsed[bindingBag->bag - bags] = true;
    } cnEnd;
  } cnEnd;

  // We finished.
  result = true;

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

bool cnVerifyImprovement_StatsPrepare(
  cnVerifyImprovement_Stats* stats, cnRootNode* tree, cnList(Bag)* bags,
  cnRandom random
) {
  cnCount classCount;
  cnIndex i;
  cnFloat* probs = NULL;
  bool result = false;

  // Gather up the original counts for each leaf.
  if (!cnTreeMaxLeafCounts(tree, &stats->leafCounts, bags)) {
    cnErrTo(DONE, "No counts.");
  }

  // Prepare place for stats.
  classCount = 2 * stats->leafCounts.count;
  probs = cnStackAllocOf(cnFloat, classCount);
  stats->bootCounts = cnAlloc(cnCount, classCount);
  if (!(probs && stats->bootCounts)) {
    cnErrTo(DONE, "No stats allocated.");
  }

  // Calculate probabilities.
  for (i = 0; i < stats->leafCounts.count; i++) {
    cnLeafCount* count =
      reinterpret_cast<cnLeafCount*>(cnListGet(&stats->leafCounts, i));
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
  )) cnErrTo(DONE, "No multinomial.")

  // Winned.
  result = true;

  DONE:
  cnStackFree(probs);
  return result;
}

bool cnVerifyImprovement(
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
  bool okay = false;
  cnVerifyImprovement_Stats previousStats;

  // Inits.
  cnVerifyImprovement_StatsInit(&candidateStats);
  cnVerifyImprovement_StatsInit(&previousStats);

  // Prepare stats for candidate and previous.
  printf("Candidate:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &candidateStats, candidate, &config->validationBags, config->learner->random
  )) cnErrTo(DONE, "No stats.");
  printf("Previous:\n");
  if (!cnVerifyImprovement_StatsPrepare(
    &previousStats, config->previous, &config->validationBags,
    config->learner->random
  )) cnErrTo(DONE, "No stats.");

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
  okay = true;

  DONE:
  // Cleanup is safe because of proper init.
  cnVerifyImprovement_StatsDispose(&candidateStats);
  cnVerifyImprovement_StatsDispose(&previousStats);

  return okay;
}


}
