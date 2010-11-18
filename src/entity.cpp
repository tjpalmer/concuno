#include "entity.h"

using namespace std;

namespace cuncuno {

Attribute::Attribute(const String& n, const Type& t, Count c):
  name(n), type(t), count(c) {}

Count Attribute::getCount(const Any& entity) const {
  return count;
}

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
