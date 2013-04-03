// Kept separate from distributions-covary.cpp because of slow Eigenvalues.

#include "matrices.cpp"

using namespace Eigen;

namespace concuno {

template double condition<double, 2, 2>(Eigen::Matrix<double, 2, 2>&, double);

}
