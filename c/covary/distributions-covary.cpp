#include "distributions.cpp"

using namespace Eigen;

namespace concuno {

template void Distribution<double, 2>::sampleRep(Matrix<double, 2, Dynamic>&);

template struct Gaussian<double, 2>;

}
