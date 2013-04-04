#include "bagclass.h"
#include "distributions.h"
#include "matrices.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace concuno;
using namespace Eigen;
using namespace std;

#define COVARY_TEST_NDIMS Dynamic
typedef Distribution<double, COVARY_TEST_NDIMS> Dist;
typedef BagBags<double, COVARY_TEST_NDIMS> Problem;
typedef Problem::Bag Bag;
typedef Problem::BagBag BagBag;
typedef Gaussian<double, COVARY_TEST_NDIMS> Model;
typedef Model::Vector Point;
typedef Model::Square Square;


void addBags(BagBag& bagBag, int bagCount, Dist& key, Dist& spray);


void buildSqueezeProblem(Problem& problem);


template<typename Scalar, int NDims>
void growVolume(const BagBags<Scalar, NDims>& problem, int firstIndex);


template<typename Scalar>
Scalar logScore(Scalar probability, int positive, int negative);


template<typename Scalar, int NDims>
Scalar minDistanceSquared(
  const Gaussian<Scalar, NDims>& model,
  const Matrix<Scalar, NDims, Dynamic>& bag,
  int* index = 0 // nullptr not accepted -- Need newer clang or some setting?
);


/**
 * After a header row indicating bag membership, prints all points from all bags
 * as if one large matrix.
 */
void printPointArray(ostream& out, const BagBag& bags);


void testProblem(void (*buildProblem)(Problem& problem));


int main() {
  switch (2) {
    case 0: {
      // Sample uniform scalars.
      Uniform<> uniform;
      for (int i = 0; i < 100; i++) {
        cout << uniform.sample() << " ";
      }
      cout << endl;
      break;
    }
    case 1: {
      // Sample Gaussian.
      Vector2d mean;
      mean << -4, 10;
      Matrix<double, 2, 2> covariance;
      covariance << 3, -1.6, -1.6, 1;
      Gaussian<double, 2> gaussian(mean, covariance);
      Matrix2Xd samples(2, 1000);
      gaussian.sample(samples);
      // Output is where lots of mallocs and frees happen.
      cout << samples << endl;
      break;
    }
    case 2: {
      // Bag classification problem.
      testProblem(buildSqueezeProblem);
      break;
    }
  }
  return 0;
}


void addBags(BagBag& bagBag, int bagCount, Dist& key, Dist& spray) {
  // TODO Randomize 1 to X total points on a per bag basis.
  int pointCount = 5;
  Point point(2, 1);
  for (int i = 0; i < bagCount; i++) {
    Bag bag(2, pointCount);
    spray.sample(bag);
    // TODO Randomize key index to avoid risks of bugs on that.
    // TODO Vary number of keys, too?
    int keyIndex = 0;
    // TODO How to sample directly into the column?
    key.sample(point);
    bag.col(keyIndex) = point;
    bagBag.push_back(bag);
  }
}


void buildSqueezeProblem(Problem& problem) {
  Point mean = Point::Zero(2);
  Square covariance = 1 * Square::Identity(2, 2);
  Model spray(mean, covariance);
  // All the others have the same mean y
  mean(1) = 2;
  int totalBags = 500;
  /* Negatives */ {
    // Left side.
    mean(0) = -1;
    // Most volume within 0.5 of 0.
    covariance << 0.25, 0, 0, 1;
    {
      Model key(mean, covariance);
      addBags(problem.negatives, totalBags / 4, key, spray);
    }
    // Right side.
    mean(0) = 1;
    // Retain variance.
    {
      Model key(mean, covariance);
      addBags(problem.negatives, totalBags / 4, key, spray);
    }
  }
  /* Positives */ {
    // Right between the negatives.
    mean(0) = 0;
    covariance = 0.25 * Square::Identity(2, 2);
    Model key(mean, covariance);
    addBags(problem.positives, totalBags / 2, key, spray);
  }
}

template<typename Scalar, int NDims>
void growVolume(const BagBags<Scalar, NDims>& problem, int firstIndex) {
  typedef Gaussian<Scalar, NDims> Model;
  typedef Matrix<Scalar, NDims, 1> Point;
  typedef Matrix<Scalar, NDims, Dynamic> Points;
  typedef BagBags<Scalar, NDims> Problem;
  typedef typename Problem::BagBag BagBag;
  typedef typename Problem::Bag Bag;
  typedef typename Model::Square Square;
  Array<bool, 1, Dynamic> positivesContained(problem.positives.size());
  const Bag& bag = problem.positives[firstIndex];
  for (int p = 0; p < bag.cols(); p++) {
    cout << p << endl;
    Point point(bag.col(p));
    int ndims = point.rows();
    Points containedPoints(point);
    positivesContained.fill(false);
    positivesContained(firstIndex) = true;
    // TODO The covariance should be chosen such that distance 1 is half way to
    // TODO nearest point among other bags.
    Model model(point, Square::Identity(ndims, ndims));
    while (!positivesContained.all()) {
      // Look for the best next bag to add.
      // We'll add the one point from the bag in question.
      // TODO Preallocate for all bags, then use a view of leftmost n?
      Points newContainedPoints(ndims, containedPoints.cols() + 1);
      newContainedPoints.leftCols(containedPoints.cols()) = containedPoints;
      // We need to find the best point for this next round.
      Scalar bestScoreStep = -numeric_limits<Scalar>::infinity();
      Model bestModelStep(model);
      Scalar bestMaxDistanceStep;
      // TODO Extract this to another function?
      // TODO I've reused var names expecting this.
      for (int b = 0; b < positivesContained.cols(); b++) {
        if (!positivesContained(b)) {
          // Find the nearest point in this bag.
          const Bag& bag = problem.positives[b];
          // Assume all bags have at least on point for now.
          // TODO Always exclude bags with no points? Prefilter out?
          int minIndex;
          minDistanceSquared(model, bag, &minIndex);
          // We have the min. Add it, update model, and calculate score.
          newContainedPoints.col(containedPoints.cols()) = bag.col(p);
          // TODO Abstract model finder (allowing iterative, too).
          Model newModel(newContainedPoints);
          Square covariance(newModel.covariance());

          // TODO What condition number is equivalent to what kinds of evidence
          // TODO vs. what kind of prior?
          Scalar maxWantedCondition(newContainedPoints.cols());
          // Eigenvalues of covariance are squared units, so square the
          // wanted condition, too.
          // This gives axis ratios proportional to evidence.
          // TODO Reduce allowance based on dimensionality?
          // TODO That is, higher dimensional systems need stronger evidence.
          maxWantedCondition *= maxWantedCondition;
          // Diagonal conditioning changes both the axis sizes and the axes
          // themselves, making it more isotropic.
          // TODO Is this what we want?
          condition(covariance, maxWantedCondition);
          newModel.covariance_put(covariance);

          // Find max distance of contained points.
          // TODO Instead of assuming including all current points, could choose
          // TODO the radius that maximizes our score, given the current shape
          // TODO and mean.
          // TODO For that, we'd likely want to sort.
          //cout << newModel.mean().transpose() << ", " << covariance.row(0) << " " << covariance.row(1) << "; ";
          //cout << newModel.mean() << endl << covariance << endl << newContainedPoints << endl;
          Scalar maxDistance = 0;
          for (int p = 0; p < newContainedPoints.cols(); p++) {
            Point point(newContainedPoints.col(p));
            Scalar distance = newModel.distanceSquared(point);
            //cout << distance << " ";
            if (distance > maxDistance) {
              maxDistance = distance;
            }
          }
          // TODO Actually, go half way between this and nearest negative bag?
          //cout << "-> " << maxDistance << endl << endl;

          // Calculate score, considering all bags with points no farther.
          int insidePositive = 0;
          for (auto& bag: problem.positives) {
            // TODO By index, could look at positivesContained to see if we
            // TODO need to bother with this calculation.
            Scalar distance = minDistanceSquared(newModel, bag);
            if (distance <= maxDistance) {
              insidePositive++;
            }
          }
          int insideNegative = 0;
          for (auto& bag: problem.negatives) {
            Scalar distance = minDistanceSquared(newModel, bag);
            if (distance <= maxDistance) {
              insideNegative++;
            }
          }
          // Now calculate grand metric (log likelihood).
          // TODO Allow option higher prob outside than in?
          int insideTotal = insidePositive + insideNegative;
          int outsideNegative = problem.negatives.size() - insideNegative;
          int outsidePositive = problem.positives.size() - insidePositive;
          int outsideTotal = outsidePositive + outsideNegative;
          double insideProbability = insidePositive / Scalar(insideTotal);
          double outsideProbability = outsidePositive / Scalar(outsideTotal);
          double score =
            logScore(insideProbability, insidePositive, insideNegative) +
            logScore(outsideProbability, outsidePositive, outsideNegative);
          // If best so far, record the model and the score.
          if (score > bestScoreStep) {
            bestScoreStep = score;
            bestModelStep = newModel;
            // TODO Alternatively, could scale the covariance for this.
            bestMaxDistanceStep = maxDistance;
          }
        }
      }
      // Got the best model for the latest round.
      //if (bestScoreStep > -310) {
      //cout << bestScoreStep << ": " << bestPosContained << " " << bestNegContained << endl << bestModelStep.mean().transpose() << endl << bestModelStep.covariance() << endl << endl;
      //}

      // Mark all contained positive bags.
      std::vector<int> nearestIndices;
      for (int b = 0; b < problem.positives.size(); b++) {
        // TODO By index, could look at positivesContained to see if we
        // TODO need to bother with this calculation.
        const Bag& bag = problem.positives[b];
        int index;
        Scalar distance = minDistanceSquared(bestModelStep, bag, &index);
        if (distance <= bestMaxDistanceStep) {
          positivesContained[b] = true;
          nearestIndices.push_back(index);
        } else if (positivesContained[b]) {
          // TODO This would be okay if we are allowed to optimize radius on
          // TODO each iteration.
          throw runtime_error("Supposedly got a positive we didn't have.");
        }
      }
      // Add the nearest point for each contained positive bag to contained
      // points.
      containedPoints.resize(ndims, nearestIndices.size());
      for (int b = 0, p = 0; b < problem.positives.size(); b++) {
        if (positivesContained[b]) {
          const Bag& bag = problem.positives[b];
          containedPoints.col(p) = bag.col(nearestIndices[p]);
          p++;
        }
      }
      // TODO Iterative might be hard, since the nearest point for each bag
      // TODO could change.
      // TODO Maybe still build a new next model immediately for speeding the
      // TODO next round of computation?
      Model nextModel(containedPoints);
      if (bestScoreStep > -310) {
        cout << bestScoreStep << ": " << nextModel.mean().transpose() << endl << nextModel.covariance() << endl;
        cout << "Was: " << bestModelStep.mean().transpose() << endl << bestModelStep.covariance() << endl;
        //cout << containedPoints << endl;
        cout << endl;
      }
      // TODO Immediately update our main model to reassess radius and score?
      // TODO Do this earlier when calculating score?
      // TODO Could do a new "tiny loop", but maybe that's overdoing it?

      // TODO If the best for the latest round is the best of all rounds, record
      // TODO that (scaled model and score) here, too.
      // TODO That's best across sampled points from first bag, too.

      // TODO Any gains from quitting early if we can't beat our existing best
      // TODO (say, even if we got all remaining positives and no negatives)?
      // TODO Try it? Check stats?

      break;
    }

    // TODO Again, keep best model and score.
  }
}


template<typename Scalar>
Scalar logScore(Scalar probability, int positive, int negative) {
  // Instead of probability ^ p * (1 - probability) ^ n, use log.
  // I probability is 1 for no negatives, we get 0 ^ 0, or -inf * 0, but really
  // the exponent is a product loop on negatives, for which there are none, so
  // check these conditions and + 0 for log space (* 1 for common).
  Scalar scorePositive = positive ? log(probability) * positive : 0;
  Scalar scoreNegative = negative ? log(1 - probability) * negative : 0;
  return scorePositive + scoreNegative;
}


template<typename Scalar, int NDims>
Scalar minDistanceSquared(
  const Gaussian<Scalar, NDims>& model,
  const Matrix<Scalar, NDims, Dynamic>& bag,
  int* index
) {
  Scalar minDistance = numeric_limits<Scalar>::infinity();
  int minIndex = -1;
  for (int p = 0; p < bag.cols(); p++) {
    Matrix<Scalar, NDims, 1> point(bag.col(p));
    Scalar distance = model.distanceSquared(point);
    if (distance < minDistance) {
      minDistance = distance;
      minIndex = p;
    }
  }
  if (index) {
    *index = minIndex;
  }
  return minDistance;
}


void printPointArray(ostream& out, const BagBag& bags) {
  // First the header row.
  int ndims = 0;
  for (BagBag::size_type bb = 0; bb < bags.size(); bb++) {
    auto& bag = bags[bb];
    if (!ndims) {
      ndims = bag.rows();
    }
    for (int j = 0; j < bag.cols(); j++) {
      out << bb + 1 << " ";
    }
  }
  out << endl;
  // Now, for each row (dimension), go through each bag.
  for (int i = 0; i < ndims; i++) {
    for (auto& bag: bags) {
      out << bag.row(i) << " ";
    }
    out << endl;
  }
}


void testProblem(void (*buildProblem)(Problem& problem)) {
  Problem problem;
  buildProblem(problem);
  for (int i = 0; i < 2; i++) {
    growVolume(problem, i);
  }
  //printPointArray(cout, problem.negatives);
  // TODO Learn decision volume.
  // TODO Iterative variance:
  // TODO http://www.johndcook.com/standard_deviation.html
  // TODO http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
  // TODO Covariance:
  // TODO http://en.wikipedia.org/wiki/Covariance_matrix
  // TODO http://en.wikipedia.org/wiki/Computational_formula_for_the_variance
}
