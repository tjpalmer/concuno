#include "bagclass.h"
#include "distributions.h"
#include <iostream>

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


void testProblem(void (*buildProblem)(Problem& problem));


int main() {
  switch (2) {
    case 1: {
      // Sample Gaussian.
      Vector2d mean;
      mean << -4, 10;
      Matrix<double, 2, 2> covariance;
      covariance << 3, -1.6, -1.6, 1;
      Gaussian<double, 2> gaussian(mean, covariance);
      Matrix2Xd samples(2, 1000);
      gaussian.sampleRep(samples);
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
    spray.sampleRep(bag);
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
  Square covariance = 2 * Square::Identity();
  Gaussian<double, 2> spray(mean, covariance);
  // All the others have the same mean y
  mean(1) = 0.5;
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
    covariance.setIdentity();
    Gaussian<double, 2> key(mean, covariance);
    addBags(problem.positives, totalBags / 2, key, spray);
  }
}


void testProblem(void (*buildProblem)(Problem& problem)) {
  Problem problem;
  buildProblem(problem);
  // TODO Learn decision volume.
}
