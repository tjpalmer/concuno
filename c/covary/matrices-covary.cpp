// Kept separate from distributions-covary.cpp because of slow Eigenvalues.

#include "matrices.cpp"

using namespace Eigen;

namespace concuno {

// TODO Move to some header.
#define COVARY_TEST_NDIMS Dynamic

template double condition<double, COVARY_TEST_NDIMS, COVARY_TEST_NDIMS>(
  Eigen::Matrix<double, COVARY_TEST_NDIMS, COVARY_TEST_NDIMS>&, double
);

}
