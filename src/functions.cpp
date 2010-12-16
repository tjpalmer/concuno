#include <cstring>
#include "functions.h"

using namespace std;

namespace cuncuno {


/// GetFunction.

GetFunction::GetFunction(
  const String& name,
  const Type& $entityType,
  const Type& $attributeType,
  Size $offset
):
  Function(name),
  entityType($entityType),
  attributeType($attributeType),
  offset($offset)
{}

void GetFunction::operator()(const void* in, void* out) const {
  memcpy(out, reinterpret_cast<const Byte*>(in) + offset, attributeType.size);
}

const Type& GetFunction::typeIn() const {
  return entityType;
}

const Type& GetFunction::typeOut() const {
  return attributeType;
}


/// PutFunction.

PutFunction::PutFunction(GetFunction& $get):
  Function($get.name + '='), get($get) {}

void PutFunction::operator()(const void* in, void* out) const {
  memcpy(reinterpret_cast<Byte*>(out) + get.offset, in, typeOut().size);
}

const Type& PutFunction::typeIn() const {
  // Reversed on purpose.
  return get.typeOut();
}

const Type& PutFunction::typeOut() const {
  // Reversed on purpose.
  return get.typeIn();
}


}
