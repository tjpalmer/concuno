#include <Eigen/LU>
#include <iostream>

using namespace Eigen;
using namespace std;


namespace covary {


/**
 * TODO Specialize for dynamic cases???
 */
template<typename Scalar = double, int Size = 1>
struct Uniform {

  typedef Matrix<Scalar, Size, 1> Vector;

  Uniform(Scalar begin = 0, Scalar end = 1);

  Uniform(const Vector& begin, const Vector& end);

  void sample(Vector& vector);

  template<int Rows, int Cols>
  void sample(Matrix<Scalar, Rows, Cols>& matrix);

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


/**
 * TODO Multivariate!
 */
template<typename Scalar, int Size>
struct Gaussian {

  typedef Matrix<Scalar, Size, Size> Covariance;

  typedef Matrix<Scalar, Size, 1> Vector;

  /**
   * From: Knuth and user173973 at http://stackoverflow.com/questions/5287009
   */
  Scalar sample() {
    // Rejection sample a point in the unit circle cetnered at the origin.
    Scalar squaredNorm;
    Uniform<Scalar> uniform(-1, 1);
    Matrix<Scalar, 2, 1> point;
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

    // TODO Multivariate.
    // TODO Mean and covariance!
    return value;
  }

};


template<typename Scalar>
struct Solver {

  typedef Matrix<Scalar, Dynamic, Dynamic> Mat;

};


}


using namespace covary;


int main(void) {
  Solver<double> solver;
  MatrixXd m = 2 * MatrixXd::Random(5, 5);
  m(0, 4) = m(4, 0) = -4;
  auto b = m.block<1,1>(0,0);
  //m.row(0) << 1, 2, 3, 4, 5;
  //  VectorXd v(5);
  //  v << 1, 2, 3, 4, 5;
  //m.row(0) = v;
  //m.row(0) << v;
  //cout << v << endl;/
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
    cout << dist.sample() << ", ";
  }
  cout << endl;
  return 0;
}


namespace covary {


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
void Uniform<Scalar, Size>::sample(Vector& vector) {
  vector = minRange.col(0) + minRange.col(1).cwiseProduct(Vector::Random());
}


template<typename Scalar, int Size>
template<int Rows, int Cols>
void Uniform<Scalar, Size>::sample(Matrix<Scalar, Rows, Cols>& matrix) {
  // TODO Validate size!
  Vector vector;
  for (typename Vector::Index i = 0; i < Rows / Size; i++) {
    for (typename Vector::Index j = 0; j < Cols; j++) {
      // TODO Thinks we have < here. How to resolve?
      // sample(matrix.block<Size, 1>(i * Size, j));
      sample(vector);
      // TODO Again, <.
      // matrix.block<Size, 1>(i * Size, j) = vector;
      matrix.block(i * Size, j, Size, 1) = vector;
    }
  }
}


}
