#ifndef concuno_distributions_cpp
#define concuno_distributions_cpp

#include "distributions.h"
#include <cstdlib>
#include <Eigen/Cholesky>
#include <Eigen/LU>

namespace concuno {


template<typename Scalar, int Size>
void updateCodeviation(
  const Eigen::Matrix<Scalar, Size, Size>& covariance,
  Eigen::Matrix<Scalar, Size, Size>& codeviation
);


template<typename Scalar, int Size>
void updatePrecision(
  const Eigen::Matrix<Scalar, Size, Size>& covariance,
  Eigen::Matrix<Scalar, Size, Size>& precision
);


template<typename Scalar, int Size>
Distribution<Scalar, Size>::~Distribution() {}


template<typename Scalar, int Size>
Scalar Distribution<Scalar, Size>::sample() {
  Vector vector;
  sample(vector);
  return vector(0);
}


template<typename Scalar, int Size>
template<int Rows, int Cols>
void Distribution<Scalar, Size>::sample(
  Eigen::Matrix<Scalar, Rows, Cols>& matrix
) {
  // TODO Validate size!
  Vector vector;
  typename Vector::Index rowRep = matrix.rows() / Size;
  typename Vector::Index cols = matrix.cols();
  for (typename Vector::Index i = 0; i < rowRep; i++) {
    for (typename Vector::Index j = 0; j < cols; j++) {
      // TODO Thinks we have < here. How to resolve?
      // sample(matrix.block<Size, 1>(i * Size, j));
      sample(vector);
      // TODO Again, <.
      // matrix.block<Size, 1>(i * Size, j) = vector;
      matrix.block(i * Size, j, Size, 1) = vector;
    }
  }
}


/**
 * Extract a scalar sample from N(0, 1).
 *
 * See Knuth and Marsaglia & Bray.
 * I originally found it from user173973 at
 * http://stackoverflow.com/questions/5287009
 */
template<typename Scalar>
Scalar gaussianSample() {
  // Rejection sample a point in the unit circle cetnered at the origin.
  Scalar squaredNorm;
  Uniform<Scalar> uniform(-1, 1);
  Eigen::Matrix<Scalar, 2, 1> point;
  do {
    uniform.sample(point);
    squaredNorm = point.squaredNorm();
  } while (squaredNorm >= 1.0);
  // Got one. Convert to N(0, 1) sample.
  Scalar value;
  if (squaredNorm) {
    value = point(0) * sqrt(-2 * log(squaredNorm) / squaredNorm);
  } else {
    value = 0;
  }
  return value;
}


template<typename Scalar, int Size>
Gaussian<Scalar, Size>::Gaussian(Scalar mean__, Scalar variance) {
  mean_.fill(mean__);
  covariance_.setZero();
  covariance_.diagonal().fill(variance);
  codeviation_.diagonal().fill(sqrt(variance));
}


template<typename Scalar, int Size>
Gaussian<Scalar, Size>::Gaussian(
  const Vector& mean__, const Square& covariance__
):
  codeviation_(mean__.rows(), mean__.rows()),
  covariance_(covariance__),
  mean_(mean__),
  precision_(mean__.rows(), mean__.rows())
{
  updateCodeviation(covariance_, codeviation_);
  updatePrecision(covariance_, precision_);
}


template<typename Scalar, int Size>
Gaussian<Scalar, Size>::Gaussian(const Points& points):
  codeviation_(points.rows(), points.rows()),
  covariance_(points.rows(), points.rows()),
  mean_(points.rowwise().mean()),
  precision_(points.rows(), points.rows())
{
  // Covariance.
  covariance_.setZero();
  for (int j = 0; j < points.cols(); j++) {
    Vector diff(points.col(j) - mean_);
    covariance_ += diff * diff.transpose();
  }
  // - 1 for unbiased.
  covariance_ /= points.cols() - 1;
  // Update codeviation and precision.
  updateCodeviation(covariance_, codeviation_);
  updatePrecision(covariance_, precision_);
}


template<typename Scalar, int Size>
Gaussian<Scalar, Size>::~Gaussian() {}


template<typename Scalar, int Size>
const typename Gaussian<Scalar, Size>::Square&
Gaussian<Scalar, Size>::covariance() const {
  return covariance_;
}


template<typename Scalar, int Size>
void Gaussian<Scalar, Size>::covariance_put(const Square& covariance) {
  covariance_ = covariance;
  updateCodeviation(covariance_, codeviation_);
  updatePrecision(covariance_, precision_);
}


template<typename Scalar, int Size>
Scalar Gaussian<Scalar, Size>::distanceSquared(Vector& vector) const {
  Vector diff = vector - mean_;
  Scalar distance = diff.transpose() * precision_ * diff;
  return distance;
}


template<typename Scalar, int Size>
const typename Gaussian<Scalar, Size>::Vector&
Gaussian<Scalar, Size>::mean() const {
  return mean_;
}


template<typename Scalar, int Size>
void Gaussian<Scalar, Size>::sample(Vector& vector) {
  for (typename Vector::Index i = 0; i < Size; i++) {
    vector(i) = gaussianSample<Scalar>();
  }
  vector = mean_ + codeviation_ * vector;
}


template<typename Scalar, int Size>
Uniform<Scalar, Size>::Uniform(Scalar begin, Scalar end) {
  minRange.col(0).fill(begin);
  minRange.col(1).fill(end - begin);
}


template<typename Scalar, int Size>
Uniform<Scalar, Size>::Uniform(const Vector& begin, const Vector& end) {
  minRange.col(0) = begin;
  minRange.col(1) = end - begin;
}


template<typename Scalar, int Size>
Uniform<Scalar, Size>::~Uniform() {}


template<typename Scalar, int Size>
void Uniform<Scalar, Size>::sample(Vector& vector) {
  // Pick values 0 to 1.
  // Don't built-in Eigen Random(), because it goes -1 to 1 for doubles.
  // We need 0 to 1.
  // TODO What to do for ints?
  for (typename Vector::Index i = 0; i < vector.rows(); i++) {
    // TODO Do better than rand!
    vector(i) = Scalar(std::rand()) / Scalar(RAND_MAX);
  }
  // Now scale.
  vector = minRange.col(0) + minRange.col(1).cwiseProduct(vector);
}


template<typename Scalar, int Size>
void updateCodeviation(
  const Eigen::Matrix<Scalar, Size, Size>& covariance,
  Eigen::Matrix<Scalar, Size, Size>& codeviation
) {
  // Square root via Cholesky.
  // See also LDLT::reconstructedMatrix for my example on how to use Eigen to
  // do this.
  auto ldlt = covariance.ldlt();
  codeviation.setIdentity();
  codeviation = ldlt.vectorD().cwiseSqrt().asDiagonal() * codeviation;
  codeviation = ldlt.matrixL() * codeviation;
  codeviation = ldlt.transpositionsP().transpose() * codeviation;
}


template<typename Scalar, int Size>
void updatePrecision(
  const Eigen::Matrix<Scalar, Size, Size>& covariance,
  Eigen::Matrix<Scalar, Size, Size>& precision
) {
  // TODO Condition covariance if (near) singular?
  precision = covariance.inverse();
}


}


#endif
