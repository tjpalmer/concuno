#include <cstring>
#include "entity.h"

using namespace std;

namespace cuncuno {


/// Attribute.

Attribute::Attribute(Function* $get, Function* $put): get($get), put($put) {
  if (!get) {
    throw "Null get.";
  }
}

const Type& Attribute::entityType() const {
  return get->typeIn();
}

const String& Attribute::name() const {
  return get->name;
}

const Type& Attribute::type() const {
  return get->typeOut();
}


/// Function.

Function::Function(const String& $name): name($name) {}


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

Type::Type(TypeSystem& $system, const String& $name, Size $size):
  name($name), base(*this), count(1), size($size), system($system) {}

Type::Type(const Type& $base, Count $count):
    base($base), count($count), size($base.size * $count), system($base.system)
{}

Type::~Type() {
  for (Count t = 0; t < arrayTypes.size(); t++) {
    delete arrayTypes[t];
  }
}

const Type& Type::arrayType(Count count) const {
  // First make sure we have enough space.
  if (arrayTypes.size() <= count) {
    const_cast<vector<const Type*>&>(arrayTypes).resize(count + 1, 0);
  }
  // Then make sure we've defined the array type. It's a needless check if we
  // just resized, but oh well. The common case is that we've already defined
  // the type.
  const Type* type = arrayTypes[count];
  if (!type) {
    type = new Type(*this, count);
    const_cast<vector<const Type*>&>(arrayTypes)[count] = type;
  }
  // And return the type.
  return *type;
}


bool Type::operator ==(const Type& type) const {
  return this == &type;
}


/// TypeSystem.

TypeSystem::TypeSystem() {
  // TODO Other cores.
  types.push_back(new Type(*this, "Float", sizeof(Float)));
}

TypeSystem::~TypeSystem() {
  for (Count t = 0; t < types.size(); t++) {
    delete types[t];
  }
}

const Type& TypeSystem::$float() {
  // TODO Constants for well-known indexes.
  return *types[0];
}

}
