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


void printPointArray(ostream& out, const BagBag& bags) {
  // First the header row.
  int ndims = 0;
  for (int bb = 0; bb < bags.size(); bb++) {
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
  //printPointArray(cout, problem.negatives);
  // TODO Learn decision volume.
}
