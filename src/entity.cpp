#include <algorithm>
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
  name($name),
  base(*this),
  count(1),
  size($size),
  system($system),
  $isPointer(false)
{}

Type::Type(const Type& other):
  name(other.name),
  base(other.base == other ? *this : other.base),
  count(other.count),
  size(other.size),
  system(other.system),
  attributes(other.attributes),
  $isPointer(other.$isPointer)
{}

Type::Type(const Type& $base, Count $count, bool $$isPointer):
  name($base.name),
  base($base),
  count($count),
  size($count * ($$isPointer ? sizeof(void*) : $base.size)),
  system($base.system),
  $isPointer($$isPointer)
{
  if (isPointer()) {
    name += "*";
  }
  if (count != 1 || !isPointer()) {
    // TODO Put in actual count.
    name += "[]";
  }
}

Type::~Type() {
  for (Count t = 0; t < arrayTypes.size(); t++) {
    delete arrayTypes[t];
  }
}

const Type& Type::arrayType(Count count) const {
  if (!count) {
    throw "Array of length zero unsupported. Use pointerType() for pointer.";
  }
  // Make sure we have enough space.
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

bool Type::isPointer() const {
  return $isPointer;
}

bool Type::operator==(const Type& type) const {
  return this == &type;
}

bool Type::operator!=(const Type& type) const {
  return !(*this == type);
}

const Type& Type::pointerType() const {
  if (!$pointerType.get()) {
    // Peripheral changes here.
    const_cast<Type*>(this)->$pointerType.reset(new Type(*this, 1, true));
  }
  return *$pointerType;
}


/// TypeSystem.

TypeSystem::TypeSystem() {
  // TODO Other cores.
  types.push_back(new Type(*this, "Float", sizeof(Float)));
  types.push_back(new Type(*this, "Int", sizeof(Int)));
  types.push_back(new Type(*this, "Count", sizeof(Count)));
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

const Type& TypeSystem::$int() {
  // TODO Constants for well-known indexes.
  return *types[1];
}

const Type& TypeSystem::count() {
  // TODO Constants for well-known indexes.
  return *types[2];
}


}
