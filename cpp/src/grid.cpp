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

void* Grid::at(Count index0, ...) {
  //Count count(this->count());
  va_list indexes;
  va_start(indexes, index0);
  for (Count d(1); d < ndim(); d++) {
    //$shape[s] = va_arg(indexes, Count);
  }
  va_end(indexes);
  return 0;
}

Count Grid::count() const {
  return list.count();
}

Count Grid::ndim() const {
  return $shape.count() + 1;
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

void* List::at(Count index) {
  // TODO
  return 0;
}

Count List::count() const {
  return $count >= 0 ? $count : -$count;
}


}
