#include <cstdlib>
#include <memory>
#include "predicate.h"

namespace cuncuno {


/// AttributePredicate.

bool FunctionPredicate::operator()(const void* entity) const {
  // TODO Some auto_ptr for arrays. Exceptions here will leak memory.
  // TODO Or just use alloca/_malloca instead?
  Byte* buffer = new Byte[function->typeOut().size];
  (*function)(entity, buffer);
  Float value = (*probabilityFunction)(buffer);
  delete[] buffer;
  return value > threshold;
}


}
