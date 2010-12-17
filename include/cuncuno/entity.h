#ifndef cuncuno_entity_h
#define cuncuno_entity_h

#include "export.h"
#include <memory>
#include <string>
#include <vector>

namespace cuncuno {

typedef unsigned char Byte;

typedef size_t Count;

/**
 * Because I don't want to commit to single vs. double.
 *
 * TODO Support bigs, rationals, imaginaries, and so on?
 */
typedef double Float;

typedef int Int;

typedef size_t Size;

typedef std::string String;

struct Type;

/**
 * TODO Combine with attribute.
 */
struct Function {

  Function(const String& name = "");

  /**
   * The out parameter can actually be in-out, such as for attribute "put"
   * functions. For now, such issues are implicit.
   */
  virtual void operator()(const void* in, void* out) const = 0;

  virtual const Type& typeIn() const = 0;

  virtual const Type& typeOut() const = 0;

  /**
   * Directly exposed because it's more convenient to say 'blah.name' than to
   * have to provide a buffer, and by that point, we know we need storage
   * somewhere. So just make it explicit.
   */
  String name;

};

/**
 * Provides values from entities. Attributes might be abstract or composite,
 * and entities might be composite, too.
 *
 * TODO Split just into raw functions or retain knowledge of paired get/put?
 */
struct Attribute {

  Attribute(Function* get, Function* put = 0);

  const Type& entityType() const;

  const String& name() const;

  const Type& type() const;

  Function* get;

  Function* put;

};

struct TypeSystem;

/**
 * TODO Arbitrary type parameters then specialized versions filled in? Type
 * TODO parameters would include at least sizes.
 */
struct Type {

  /**
   * Builds a new base type. Defaults count to 1.
   */
  Type(TypeSystem& system, const String& name, Size size);

  /**
   * Copy constructor.
   */
  Type(const Type& other);

  /**
   * To create a derived type with particular arity or as a pointer.
   */
  Type(const Type& base, Count count, bool isPointer = false);

  /**
   * Deletes all referenced array types.
   */
  ~Type();

  /**
   * Provides a derived type with the given count, creating it if necessary,
   * and storing it in arrayTypes at an index equivalent to the count.
   *
   * Don't use this technique to make really large array types unless you want
   * a bunch of useless memory allocated for unused array type pointers in
   * between.
   *
   * This allows arrays of count 0 and 1.
   *
   * The function is labeled const because this type itself doesn't change,
   * despite a potentially added derived type.
   *
   * TODO Use count 0 to indicate dynamic count? How to support dynamic count?
   */
  const Type& arrayType(Count count) const;

  /**
   * Whether this type represents a pointer to the base type.
   */
  bool isPointer() const;

  /**
   * Simply whether the two types are at the same memory address. Deep
   * comparison isn't worth it, and type names are risky, too.
   */
  bool operator==(const Type& type) const;

  /**
   * Silly C++ making me define this.
   */
  bool operator!=(const Type& type) const;

  /**
   * Provides a pointer type for this, creating it if needed.
   */
  const Type& pointerType() const;

  String name;

  /**
   * Same as this if not based on another type.
   */
  const Type& base;

  /**
   * TODO Is 0 best for dynamic size indicator?
   */
  Count count;

  /**
   * The size of this type. If it has a base, it should be count * base.size.
   * All entities are expected to be fixed size for any particular type.
   */
  Size size;

  TypeSystem& system;

  /**
   * These might be raw values or abstract concepts and functions.
   *
   * TODO Make this a map? Easy enough to iterate but also allows fast lookup.
   */
  std::vector<Attribute> attributes;

private:

  /**
   * A controlled list of derived types of different counts to avoid
   * duplication and allow easy cleanup. These are all deleted in the
   * destructor of Type.
   */
  std::vector<const Type*> arrayTypes;

  /**
   * Whether this type is a pointer to its base type. A bit vector of flags
   * might be useful later, so this is kept private for now.
   */
  bool $isPointer;

  /**
   * The type representing a pointer to this type.
   */
  std::auto_ptr<const Type> $pointerType;

};

/**
 * Convenient access to common types along with a predefined cleanup strategy.
 */
struct TypeSystem {

  TypeSystem();

  ~TypeSystem();

  const Type& $bool();

  const Type& byte();

  const Type& $float();

  const Type& $int();

  std::vector<Type*> types;

};

/**
 * For reference counted management.
 */
struct Shared {

  Shared();

  virtual ~Shared();

  void acquire();

  void release();

private:

  Count refCount;

};

}

#endif
