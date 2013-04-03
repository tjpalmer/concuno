#include "bagclass.h"
#include "distributions.h"
#include <iostream>
#include <limits>

using namespace concuno;
using namespace Eigen;
using namespace std;


typedef Distribution<double, 2> Dist;
typedef BagBags<double, 2> Problem;
typedef Problem::Bag Bag;
typedef Problem::BagBag BagBag;
typedef Gaussian<double, 2>::Vector Point;
typedef Gaussian<double, 2>::Square Square;


void addBags(BagBag& bagBag, int bagCount, Dist& key, Dist& spray);


void buildSqueezeProblem(Problem& problem);


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
  Point point;
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
  Point mean = Point::Zero();
  Square covariance = 1 * Square::Identity();
  Gaussian<double, 2> spray(mean, covariance);
  // All the others have the same mean y
  mean(1) = 2;
  int totalBags = 500;
  /* Negatives */ {
    // Left side.
    mean(0) = -1;
    // Most volume within 0.5 of 0.
    covariance << 0.25, 0, 0, 1;
    {
      Gaussian<double, 2> key(mean, covariance);
      addBags(problem.negatives, totalBags / 4, key, spray);
    }
    // Right side.
    mean(0) = 1;
    // Retain variance.
    {
      Gaussian<double, 2> key(mean, covariance);
      addBags(problem.negatives, totalBags / 4, key, spray);
    }
  }
  /* Positives */ {
    // Right between the negatives.
    mean(0) = 0;
    covariance = 0.25 * Square::Identity();
    Gaussian<double, 2> key(mean, covariance);
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
  positivesContained.fill(false);
  positivesContained(firstIndex) = true;
  const Bag& bag = problem.positives[firstIndex];
  for (int p = 0; p < bag.cols(); p++) {
    Point point(bag.col(p));
    int ndims = point.rows();
    Points containedPoints(point);
    // TODO The covariance should be chosen such that distance 1 is half way to
    // TODO nearest point among other bags.
    Model model(point, Square::Identity(ndims, ndims));
    while (!positivesContained.all()) {
      // Look for the best next bag to add.
      // We'll add the one point from the bag in question.
      containedPoints.resize(ndims, containedPoints.cols() + 1);
      // TODO Extract this to another function?
      // TODO I've reused var names expecting this.
      for (int b = 0; b < positivesContained.cols(); b++) {
        if (!positivesContained(b)) {
          // Find the nearest point in this bag.
          const Bag& bag = problem.positives[b];
          // Assume all bags have at least on point for now.
          // TODO Always exclude bags with no points? Prefilter out?
          Scalar minDistance = numeric_limits<Scalar>::infinity();
          int minIndex = -1;
          for (int p = 0; p < bag.cols(); p++) {
            Point point(bag.col(p));
            Scalar distance = model.distanceSquared(point);
            if (distance < minDistance) {
              minDistance = distance;
              minIndex = p;
            }
          }
          // We have the min. Add it, update model, and calculate score.
          containedPoints.col(containedPoints.cols() - 1) = bag.col(p);
          // TODO Abstract model finder (allowing iterative, too).
          Model newModel(containedPoints);
          //cout << containedPoints << endl;
          //cout << newModel.covariance() << endl << endl;
          // TODO Find max distance of contained points.
          // TODO Calculate score, considering all bags with points no farther.
          // TODO If best so far, record the model and the score.
        }
      }
      // Got the best model for the latest round.

      // TODO Mark all contained positive bags.
      // TODO Add the nearest point for each contained positive bag to
      // TODO contained points.
      // TODO For iterative, we might want to mark which are the new points.
      // TODO Maybe build a new next model immediately for speeding the next
      // TODO round of computation?

      // TODO If the best for the latest round is the best of all rounds, record
      // TODO that (scaled model and score) here, too.
      // TODO That's best across sampled points from first bag, too.

      // TODO Any gains from quitting early if we can't beat our existing best
      // TODO (say, even if we got all remaining positives and no negatives)?
      // TODO Try it? Check stats?

      break;
    }
  }
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
  growVolume(problem, 0);
  //printPointArray(cout, problem.negatives);
  // TODO Learn decision volume.
  // TODO Iterative variance:
  // TODO http://www.johndcook.com/standard_deviation.html
  // TODO http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
  // TODO Covariance:
  // TODO http://en.wikipedia.org/wiki/Covariance_matrix
  // TODO http://en.wikipedia.org/wiki/Computational_formula_for_the_variance
}
