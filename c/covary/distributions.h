#ifndef concuno_distributions_h
#define concuno_distributions_h

#include <Eigen/Cholesky>
#include <Eigen/LU>

namespace covary {


/**
 * Probability distribution over some vector space.
 */
template<typename Scalar = double, int Size = 1>
struct Distribution {

  typedef Eigen::Matrix<Scalar, Size, 1> Vector;

  virtual ~Distribution();

  virtual void sample(Vector& vector) = 0;

  /**
   * Returns the first element of a vector sample, so this is likely only
   * meaningful for size 1.
   *
   * TODO Does sampling a norm ever matter? Redefine this to that?
   */
  Scalar sample1();

  /**
   * Samples repeatedly to fill the matrix.
   * Probably should have a matrix with row count as a multiple of the size.
   *
   * TODO This should be independent of any particular distribution!
   */
  template<int Rows, int Cols>
  void sampleRep(Eigen::Matrix<Scalar, Rows, Cols>& matrix);

};


/**
 * TODO Multivariate!
 */
template<typename Scalar = double, int Size = 1>
struct Gaussian: virtual Distribution<Scalar, Size> {

  typedef Eigen::Matrix<Scalar, Size, Size> Square;

  typedef typename Distribution<Scalar, Size>::Vector Vector;

  /**
   * Presumes the same mean and isotropic covariance in all dimensions.
   */
  Gaussian(Scalar mean = 0, Scalar variance = 1);

  Gaussian(const Vector& mean, const Square& covariance);

  virtual ~Gaussian();

  virtual void sample(Vector& vector);

private:

  Square codeviation;

  Square covariance;

  Vector mean;

};


/**
 * TODO Specialize for dynamic cases???
 */
template<typename Scalar = double, int Size = 1>
struct Uniform: virtual Distribution<Scalar, Size> {

  typedef typename Distribution<Scalar, Size>::Vector Vector;

  Uniform(Scalar begin = 0, Scalar end = 1);

  Uniform(const Vector& begin, const Vector& end);

  virtual ~Uniform();

  virtual void sample(Vector& vector);

private:

  /**
   * Min value column followed by range size column.
   * I might prefer separate fields for min and range, but it seems this might
   * be more efficient for the Dynamic case.
   *
   * TODO Keep such implementation details private?
   */
  typedef Eigen::Matrix<Scalar, Size, 2> MinRange;

  MinRange minRange;

};


}

// TODO Any better way to get at these?
// TODO What's proper C++ template awesomeness?
#include "distributions.cpp"

#endif
