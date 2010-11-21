#include <cstdlib>
#include <memory>
#include "predicate.h"

namespace cuncuno {


/// AttributePredicate.

bool AttributePredicate::operator ()(const void* entity) const {
  // TODO Some auto_ptr for arrays. Exceptions here will leak memory.
  Byte* buffer = new Byte[
    attribute->getCount(entity) * attribute->type.size
  ];
  attribute->get(entity, buffer);
  Float value = (*probabilityFunction)(buffer);
  delete[] buffer;
  return value > threshold;
}


}
