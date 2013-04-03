#ifndef concuno_distributions_cpp
#define concuno_distributions_cpp

#include "matrices.h"
#include <Eigen/Eigenvalues>

namespace concuno {


template<typename Scalar, int Rows, int Cols>
Scalar condition(Eigen::Matrix<Scalar, Rows, Cols>& matrix, Scalar maxWanted) {
  // TODO Separate case for self-adjoint matrices, which would be the case for
  // TODO covariance matrices.
  // TODO matrix.selfadjointView<Lower>().eigenvalues();
  auto values = matrix.eigenvalues().cwiseAbs();
  Scalar max = values.maxCoeff();
  Scalar min = values.minCoeff();
  // TODO Should I be using the signed min and max eigenvalues?
  // TODO I'm not sure how to deal generally with complex values if so.
  Scalar condition = max / min;

  if (condition > maxWanted) {
    // TODO If maxWanted is (near?) 1, then just set to identity?
    Scalar bonus = (max - min * maxWanted) / (maxWanted - 1);
    matrix.diagonal() = matrix.diagonal().array() + bonus;
  }

  return condition;
}


}

#endif
