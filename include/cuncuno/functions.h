#ifndef cuncuno_functions_h
#define cuncuno_functions_h

#include "entity.h"

namespace cuncuno {

struct ComposedFunction: Function {

  ComposedFunction(Function& outer, Function& inner);

  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  Function& outer;

  Function& inner;

};

/**
 * Simple vector difference on floats.
 *
 * This is about 2-3x slower on my computer than just manually doing subtraction
 * on float arrays, when applying optimizations in both cases.
 * TODO Is it due to data copy?
 *
 * TODO Templatize for other than Float?
 */
struct DifferenceFunction: Function {

  /**
   * The vector type for both inputs and for the single output. Should have
   * type.system.$float as a base type.
   */
  DifferenceFunction(const Type& type);

  virtual void operator()(const void* in, void* out) const;

  /**
   * A type array of 2.
   */
  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  const Type& type;

};

/**
 * A p-norm distance metric based on vector differences.
 */
struct DistanceFunction: Function {

  DistanceFunction(const Type& type, Float exponent = 2);

  virtual void operator()(const void* in, void* out) const;

  virtual const Type& typeIn() const;

  virtual const Type& typeOut() const;

  const Type& type;

};

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
 * Decorator function for pointer indirection before calling the base.
 */
struct PointerFunction: Function {

  PointerFunction(Function& base);

  virtual void operator()(const void* in, void* out) const;

  /**
   * The entity type.
   */
  virtual const Type& typeIn() const;

  /**
   * The attribute type.
   */
  virtual const Type& typeOut() const;

  Function& base;

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

}

#endif
