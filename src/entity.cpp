#include "entity.h"

using namespace std;

namespace cuncuno {


/// Attribute.

Attribute::Attribute(const String& n, const Type& t, Count c):
  name(n), type(t), count(c) {}

Count Attribute::getCount(const void* entity) const {
  return count;
}


/// Shared.

Shared::Shared(): refCount(1) {}

Shared::~Shared() {}

void Shared::acquire() {
  refCount++;
}

void Shared::release() {
  refCount--;
  if (refCount < 1) {
    delete this;
  }
}


/// Type.

const Type& Type::$float() {
  static Type floatType;
  floatType.name = "Float";
  floatType.size = sizeof(Float);
  return floatType;
}

bool Type::operator ==(const Type& type) const {
  return this == &type;
}


}
