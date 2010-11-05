#include <cstdlib>
#include "cuncuno.h"
#include <Eigen/Dense>
#include <iostream>
#include <string>

using namespace Eigen;
using namespace std;

/**
 * The main method provides command-line access to the cuncuno system. Does
 * that include minimal interactivity? I guess it depends on the file format.
 * If command-based, that comes by default.
 */
int main(int argc, char** argv) {
  // Example LS solve:
  //  MatrixXf A = MatrixXf::Random(20,4);
  //  VectorXf b = VectorXf::Random(20);
  //  VectorXf x = A.jacobiSvd(ComputeThinU|ComputeThinV).solve(b);
  //  cout << A << endl << endl;
  //  cout << b << endl << endl;
  //  cout << x << endl << endl;
}
