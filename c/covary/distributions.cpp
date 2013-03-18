#ifndef concuno_distributions_cpp
#define concuno_distributions_cpp

#include "distributions.h"

namespace covary {


template<typename Scalar, int Size>
Distribution<Scalar, Size>::~Distribution() {}


template<typename Scalar, int Size>
Scalar Distribution<Scalar, Size>::sample1() {
  Vector vector;
  sample(vector);
  return vector(0);
}


template<typename Scalar, int Size>
template<int Rows, int Cols>
void Distribution<Scalar, Size>::sampleRep(
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
    uniform.sampleRep(point);
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
Gaussian<Scalar, Size>::Gaussian(Scalar mean_, Scalar variance) {
  mean.fill(mean_);
  covariance.setZero();
  covariance.diagonal().fill(variance);
  codeviation.diagonal().fill(sqrt(variance));
}


template<typename Scalar, int Size>
Gaussian<Scalar, Size>::Gaussian(
  const Vector& mean_, const Square& covariance_
):
  codeviation(mean_.rows(), mean_.rows()), covariance(covariance_), mean(mean_)
{
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
Gaussian<Scalar, Size>::~Gaussian() {}


template<typename Scalar, int Size>
void Gaussian<Scalar, Size>::sample(Vector& vector) {
  for (typename Vector::Index i = 0; i < Size; i++) {
    vector(i) = gaussianSample<Scalar>();
  }
  vector = mean + codeviation * vector;
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
  vector = minRange.col(0) + minRange.col(1).cwiseProduct(Vector::Random());
}


}


#endif
