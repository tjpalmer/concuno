#ifndef concuno_distributions_h
#define concuno_distributions_h

#include <Eigen/Core>

namespace concuno {


/**
 * Probability distribution over some possibly multivariate vector space.
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
 * Multivariate Gaussian (normal) distribution.
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
 * Multivariate uniform distribution with different bounds available to
 * different dimensions.
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

#endif
