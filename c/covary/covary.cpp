#include "distributions.h"
#include <iostream>

using namespace covary;
using namespace Eigen;
using namespace std;


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
