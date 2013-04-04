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
  Scalar sample();

  /**
   * Samples repeatedly to fill the matrix.
   * Probably should have a matrix with row count as a multiple of the size.
   *
   * TODO This should be independent of any particular distribution!
   */
  template<int Rows, int Cols>
  void sample(Eigen::Matrix<Scalar, Rows, Cols>& matrix);

};


/**
 * Multivariate Gaussian (normal) distribution.
 */
template<typename Scalar = double, int Size = 1>
struct Gaussian: virtual Distribution<Scalar, Size> {

  /**
   * TODO I sort of like Point more than Vector. Change throughout?
   */
  typedef Eigen::Matrix<Scalar, Size, Eigen::Dynamic> Points;

  typedef Eigen::Matrix<Scalar, Size, Size> Square;

  /**
   * TODO GCC requires this to be a typedef instead of a using. Drop GCC?
   */
  typedef typename Distribution<Scalar, Size>::Vector Vector;

  /**
   * Presumes the same mean and isotropic covariance in all dimensions.
   */
  Gaussian(Scalar mean = 0, Scalar variance = 1);

  Gaussian(const Vector& mean, const Square& covariance);

  /**
   * Calculates a maximum likelihood estimate for the given points, using an
   * unbiased n-1 denominator.
   */
  Gaussian(const Points& points);

  virtual ~Gaussian();

  const Square& covariance() const;

  void covariance_put(const Square& covariance);

  /**
   * The squared Mahalanobis distance from the mean.
   */
  Scalar distanceSquared(Vector& vector) const;

  const Vector& mean() const;

  virtual void sample(Vector& vector);

  using Distribution<Scalar, Size>::sample;

private:

  /**
   * Approximate square root of the (co)variance.
   * A Cholesky decomposition is used for this, yielding odd results but still
   * useful for sampling from the distribution.
   */
  Square codeviation_;

  Square covariance_;

  Vector mean_;

  /**
   * Inverse (co)variance.
   */
  Square precision_;

};


/**
 * Multivariate uniform distribution with different bounds available to
 * different dimensions.
 */
template<typename Scalar = double, int Size = 1>
struct Uniform: virtual Distribution<Scalar, Size> {

  /**
   * TODO GCC requires this to be a typedef instead of a using. Drop GCC?
   */
  typedef typename Distribution<Scalar, Size>::Vector Vector;

  Uniform(Scalar begin = 0, Scalar end = 1);

  Uniform(const Vector& begin, const Vector& end);

  virtual ~Uniform();

  virtual void sample(Vector& vector);

  using Distribution<Scalar, Size>::sample;

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
