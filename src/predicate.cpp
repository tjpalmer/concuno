#include <cstdlib>
#include <memory>
#include "predicate.h"

namespace cuncuno {


/// AttributePredicate.

bool AttributePredicate::evaluate(const void* entity) const {
  std::auto_ptr<void> buffer(std::malloc(
    attribute->getCount(entity) * attribute->type.size
  ));
  attribute->get(entity, buffer.get());
  Float value = probabilityFunction->evaluate(buffer);
  return value > threshold;
}


}
