#ifndef concuno_setclass_h
#define concuno_bagclass_h

#include <Eigen/Core>
#include <map>
#include <vector>

namespace concuno {

/**
 * Positive and negative labeled sets of sets.
 *
 * TODO Supporting arbitrary classes could be nice, but this is better for SMRF
 * TODO needs.
 * TODO Arbitrary classes could allow dynamically finding, say, one volume that
 * TODO separates out two classes from the others.
 */
template<typename Scalar, int NDims>
struct BagBags {

  /**
   * Multiple vectors might be the same.
   *
   * We could templatize the bag size, too, but dynamic should be by far the
   * most common case.
   */
  typedef Eigen::Matrix<Scalar, NDims, Eigen::Dynamic> Bag;

  /**
   * All bags must have the same vector size.
   * If static-sized, that's enforced automatically, but not if dynamic.
   *
   * Multiple vector bags might be the same.
   */
  typedef std::vector<Bag> BagBag;

  BagBag negatives;

  BagBag positives;

};


template<typename Scalar, int Size>
struct LabeledPoint {

  typedef Eigen::Matrix<Scalar, Size, 1> Point;

  bool label;

  Point point;

};


}

#endif

