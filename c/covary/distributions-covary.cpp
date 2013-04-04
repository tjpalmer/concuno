#include "distributions.cpp"

using namespace Eigen;

namespace concuno {

// TODO Move to some header.
#define COVARY_TEST_NDIMS Dynamic

template double Distribution<double, 1>::sample();

template void Distribution<double, COVARY_TEST_NDIMS>::sample(
  Matrix<double, COVARY_TEST_NDIMS, Dynamic>&
);

template struct Distribution<double, COVARY_TEST_NDIMS>;

template struct Gaussian<double, COVARY_TEST_NDIMS>;

template struct Uniform<double, 1>;

}
