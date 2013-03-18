#include <Eigen/Cholesky>
#include <Eigen/LU>
// Anything using Eigen/Eigenvalues compiles SLOWLY!
//#include <Eigen/MatrixFunctions>
#include <iostream>

using namespace Eigen;
using namespace std;


namespace covary {


/**
 * Probability distribution over some vector space.
 */
template<typename Scalar = double, int Size = 1>
struct Distribution {

  typedef Matrix<Scalar, Size, 1> Vector;

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
  void sampleRep(Matrix<Scalar, Rows, Cols>& matrix);

};


/**
 * TODO Multivariate!
 */
template<typename Scalar = double, int Size = 1>
struct Gaussian: virtual Distribution<Scalar, Size> {

  typedef Matrix<Scalar, Size, Size> Covariance;

  typedef typename Distribution<Scalar, Size>::Vector Vector;

  /**
   * Presumes the same mean and isotropic covariance in all dimensions.
   */
  Gaussian(Scalar mean = 0, Scalar variance = 1);

  Gaussian(const Vector& mean, const Covariance& covariance);

  virtual ~Gaussian();

  virtual void sample(Vector& vector);

private:

  Covariance codeviation;

  Covariance covariance;

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
  typedef Matrix<Scalar, Size, 2> MinRange;

  MinRange minRange;

};


template<typename Scalar>
struct Solver {

  typedef Matrix<Scalar, Dynamic, Dynamic> Mat;

};


}


using namespace covary;


int main(void) {
  if (false) {
    //Solver<double> solver;
    MatrixXd m = 2 * MatrixXd::Identity(5, 5); //Random(5, 5);
    m(0, 4) = m(4, 0) = -4;
    //auto b = m.block<1,1>(0,0);
    //m.row(0) << 1, 2, 3, 4, 5;
    //  VectorXd v(5);
    //  v << 1, 2, 3, 4, 5;
    //m.row(0) = v;
    //m.row(0) << v;
    //cout << v << endl;
    cout << "Here is the MatrixXd m:" << endl << m << endl;
    MatrixXd i = m.inverse();
    cout << "Its inverse is:" << endl << i << endl;
    MatrixXd p = m * i;
    cout << "Their product is:" << endl << p << endl;
    //  cout
    //    << "The product of " << m << " and " << v << " is "
    //    << endl << (m * v) << endl;
    //  cout << (v.transpose() * m) << endl;
    Gaussian<double, 1> dist;
    for (int i = 0; i < 100; i++) {
      cout << dist.sample1() << ", ";
    }
    cout << endl;
    MatrixXd square = m * m;
    cout << square << endl;
    auto ldlt = square.ldlt();
    // See LDLT::reconstructedMatrix
    MatrixXd root = MatrixXd::Identity(square.rows(), square.cols());
    root = ldlt.vectorD().cwiseSqrt().asDiagonal() * root;
    root = ldlt.matrixL() * root;
    root = ldlt.transpositionsP().transpose() * root;
    //MatrixXd root = square.sqrt();
    cout << root << endl;
    cout << (root * root.transpose()) << endl;
  }

  if (true) {
    Vector2d mean;
    mean << -4, 10;
    Matrix<double, 2, 2> covariance;
    covariance << 3, -1.6, -1.6, 1;
    Gaussian<double, 2> gaussian(mean, covariance);
    Matrix2Xd samples(2, 5000);
    gaussian.sampleRep(samples);
    // Output is where lots of mallocs and frees happen.
    cout << samples << endl;
  }
  return 0;
}


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
void Distribution<Scalar, Size>::sampleRep(Matrix<Scalar, Rows, Cols>& matrix) {
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
  Matrix<Scalar, 2, 1> point;
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
  const Vector& mean_, const Covariance& covariance_
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
