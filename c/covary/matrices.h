#ifndef concuno_matrices_h
#define concuno_matrices_h

#include <Eigen/Core>

namespace concuno {


/**
 * If the matrix condition number is above the value given, add a value to the
 * diagonal such that it has the given condition number.
 *
 * This uses abs(max(eigenvalues) / min(eigenvalues)) as the condition number
 * calculation.
 *
 * Returns the old condition number from before any changes.
 *
 * Note that the default requested max condition is infinite, so leaving off
 * that parameter results in retrieving the value and not changing anything.
 * TODO Still want a const version separate from this!
 */
template<typename Scalar, int Rows, int Cols>
Scalar condition(
  Eigen::Matrix<Scalar, Rows, Cols>& matrix,
  Scalar maxWanted = std::numeric_limits<Scalar>::infinity()
);


}

#endif
