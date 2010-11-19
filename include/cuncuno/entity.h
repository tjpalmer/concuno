#ifndef cuncuno_entity_h
#define cuncuno_entity_h

#include "export.h"
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
 * Provides values from entities. Attributes might be abstract or composite,
 * and entities might be composite, too.
 *
 * TODO Unify attribute with Function.
 */
struct Attribute {

  Attribute(const String& name, const Type& type, Count count = 1);

  /**
   * Gets the value of the attribute for the given entity. The buffer must be
   * large enough to store the data.
   */
  virtual void get(const void* entity, void* buffer) const = 0;

  /**
   * The count, dependent on a particular entity. By default, it defers to the
   * constant count.
   *
   * Note that the entity-specific count could actually be zero.
   *
   * TODO Would this ever require much calculation to determine? If so, allow
   * TODO expandable output or partial evaluations for efficiency in creating
   * TODO output buffers to match.
   */
  virtual Count getCount(const void* entity) const;

  const String name;

  const Type& type;

  /**
   * To allocate large arrays efficiently, check for constant count first here.
   *
   * If zero, then it depends on each specific entity. Presumably you'd never
   * want to define an always-empty attribute.
   */
  const Count count;

};

struct Type {

  static const Type& $bool();

  static const Type& byte();

  static const Type& $float();

  static const Type& $int();

  /**
   * Simply whether the two types are at the same memory address. Deep
   * comparison isn't worth it, and type names are risky, too.
   */
  bool operator ==(const Type& type) const;

  String name;

  /**
   * The size of one instance of this type. All entities are expected to be
   * fixed size for any particular type.
   *
   * TODO Do I really care about this???
   */
  Size size;

  /**
   * These might be raw values or abstract concepts and functions.
   *
   * TODO Make this a map? Easy enough to iterate but also allows fast lookup.
   */
  std::vector<Attribute*> attributes;

};

template<typename Value>
struct Metric {

  virtual void difference(const Value& a, const Value& b, Value& diff);

  virtual Float distance(const Value& a, const Value& b);

};

}

#endif
