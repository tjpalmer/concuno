#include <cstdlib>
#include "grid.h"

namespace cuncuno {

bool needsBuffer(Grid& grid) {
  // TODO
  return false;
}

Grid::Grid(const Type& t): type(t) {}

Grid::~Grid() {
  if (needsBuffer(*this)) {
    std::free(data.buffer);
  }
}

}
