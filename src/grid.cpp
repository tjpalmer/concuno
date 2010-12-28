#include <cstdlib>
#include "grid.h"
#include <stdarg.h>


namespace cuncuno {


bool needsBuffer(List& grid) {
  // TODO
  return false;
}


/// Grid.

Grid::Grid(const Type& t): $shape(ListOf<Count>(t.system.count())), list(t) {}

Grid::Grid(const Type& type, Count ndim, ...):
  $shape(ListOf<Count>(type.system.count(), ndim)), list(type)
{
  va_list sizes;
  va_start(sizes, ndim);
  for (Count s(0); s < ndim; s++) {
    $shape[s] = va_arg(sizes, Count);
  }
  va_end(sizes);
}

Count Grid::count() const {
  return list.count();
}

const Type& Grid::type() const {
  return list.type;
}


/// List.

List::List(const Type& t): type(t), $count(0) {}

List::List(const Type& t, Count c): type(t), $count(c) {}

List::~List() {
  if (needsBuffer(*this)) {
    std::free(data.buffer);
  }
}

Count List::count() const {
  return $count >= 0 ? $count : -$count;
}


}
