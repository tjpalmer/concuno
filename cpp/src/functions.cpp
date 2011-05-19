#include <cstring>
#include "functions.h"

using namespace std;

namespace cuncuno {


/// ComposedFunction.

ComposedFunction::ComposedFunction(Function& $outer, Function& $inner):
  Function($outer.name + $inner.name), outer($outer), inner($inner)
{
  if (outer.typeIn() == inner.typeOut()) {
    // Raw composition.
    if (outer.typeIn().count != inner.typeOut().count) {
      throw "Outer count in != inner count out.";
    }
  } else {
    // Array composition.
    if (outer.typeIn().base != inner.typeOut()) {
      throw "Incompatible types for array composition.";
    }
    // Call early to preallocate the array types, if needed.
    typeIn();
    inner.typeOut().arrayType(outer.typeIn().count);
  }
}

void ComposedFunction::operator()(const void* in, void* out) const {
  if (outer.typeIn() == inner.typeOut()) {
    vector<Byte> buffer(inner.typeOut().size);
    inner(in, &buffer.front());
    outer(&buffer.front(), out);
  } else {
    const Type& innerIn(inner.typeIn());
    const Type& innerOut(inner.typeOut());
    const Type& outerIn(innerOut.arrayType(outer.typeIn().count));
    // Gather up all the inner outs for a single call to outer.
    vector<Byte> buffer(outerIn.size);
    for (Count i(0); i < outerIn.count; i++) {
      inner(
        reinterpret_cast<const Byte*>(in) + i * innerIn.size,
        &buffer[i * innerOut.size]
      );
    }
    outer(&buffer.front(), out);
  }
}

const Type& ComposedFunction::typeIn() const {
  // Determine raw or array.
  if (outer.typeIn() == inner.typeOut()) {
    // Just go raw.
    return inner.typeIn();
  } else {
    // Need an array, one for each inner invocation.
    return inner.typeIn().arrayType(outer.typeIn().count);
  }
}

const Type& ComposedFunction::typeOut() const {
  return outer.typeOut();
}


/// DifferenceFunction.

DifferenceFunction::DifferenceFunction(const Type& $type):
  Function("Difference"), type($type)
{
  // Preallocate in case it's needed.
  type.arrayType(2);
  if (type.base != type.system.$float()) {
    throw "Difference only supported on floats for now.";
  }
}

void DifferenceFunction::operator()(const void* in, void* out) const {
  Count count(type.count);
  const Float* a(reinterpret_cast<const Float*>(in));
  const Float* b(reinterpret_cast<const Float*>(in) + count);
  Float* c(reinterpret_cast<Float*>(out));
  for (Count i = 0; i < count; i++) {
    c[i] = a[i] - b[i];
  }
}

const Type& DifferenceFunction::typeIn() const {
  return type.arrayType(2);
}

const Type& DifferenceFunction::typeOut() const {
  return type;
}


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


/// PointerFunction.

PointerFunction::PointerFunction(Function& $base):
  Function($base.name), base($base)
{
  // Preallocate the pointer type.
  typeIn();
}

void PointerFunction::operator()(const void* in, void* out) const {
  in = *reinterpret_cast<const void *const *>(in);
  base(in, out);
}

const Type& PointerFunction::typeIn() const {
  return base.typeIn().pointerType();
}

const Type& PointerFunction::typeOut() const {
  return base.typeOut();
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
