#include "distributions.cpp"

using namespace Eigen;

namespace concuno {

template double Distribution<double, 1>::sample();

template void Distribution<double, 2>::sample(Matrix<double, 2, Dynamic>&);

template struct Gaussian<double, 2>;

template struct Uniform<double, 1>;

}
