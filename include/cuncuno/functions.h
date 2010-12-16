#ifndef cuncuno_functions_h
#define cuncuno_functions_h

#include "entity.h"

namespace cuncuno {

/**
 * Simple getter for the common case of a struct with memory readable at a
 * particular offset.
 */
struct GetFunction: Function {

  /**
   * The name should be the name of the attribute.
   */
  GetFunction(
    const String& name,
    const Type& entityType,
    const Type& attributeType,
    Size offset
  );

  virtual void operator()(const void* in, void* out) const;

  /**
   * The entity type.
   */
  virtual const Type& typeIn() const;

  /**
   * The attribute type.
   */
  virtual const Type& typeOut() const;

  const Type& entityType;

  const Type& attributeType;

  /**
   * Offset in bytes from the start of the entity.
   */
  const Size offset;

};

/**
 * Convenience put function mirroring a convenience get function.
 */
struct PutFunction: Function {

  /**
   * The name will be 'AttributeName='.
   */
  PutFunction(GetFunction& get);

  /**
   * The value goes in. The entity goes inout via out.
   */
  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  GetFunction& get;

};

/**
 * Simple vector difference on floats.
 */
struct DifferenceFunction: Function {

  DifferenceFunction(const Type& type);

  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  const Type& type;

};

struct DistanceFunction: Function {

  DistanceFunction(const Type& type);

  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  const Type& type;

};

struct ComposedFunction: Function {

  ComposedFunction(Function& outer, Function& inner);

  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  Function& outer;

  Function& inner;

};

}

#endif
