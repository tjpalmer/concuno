#ifndef cuncuno_entity_h
#define cuncuno_entity_h

#include "export.h"
#include <string>
#include <vector>

namespace cuncuno {

typedef int Category;

typedef double Float;

typedef int Id;

typedef int TypeId;

struct Entity {

  Entity();

  Id id;

  TypeId typeId;

};

template<typename Value>
struct Metric {

  virtual void difference(Value* a, Value* b, Value* diff, size_t count);

  virtual Float distance(Value* a, Value* b, size_t count);

};

template<typename Value>
struct Attribute {

  /**
   * All attributes of a given kind are expected to be the same size, not
   * dependent on the specific entity. It allows matrix allocation and so on.
   *
   * TODO Introduce a different kind of attribute if we run across needs for
   * TODO bags at the attribute level?
   */
  virtual size_t count() const = 0;

  virtual void get(const Entity* entity, Value* values) const = 0;

  virtual void name(std::string& buffer) const = 0;

};

/**
 * TODO How to identify at runtime vs. ordinal?
 */
typedef Attribute<Category> CategoryAttribute;

typedef Attribute<Float> FloatAttribute;

/**
 * TODO Or rename EntityType to TypeId and this to Type?
 * TODO Could have one schema cover multiple types, ideally.
 */
struct Schema {
  // TODO Destructor to delete attribute pointers?
  Id id;
  std::vector<CategoryAttribute*> categoryAttributes;
  std::vector<FloatAttribute*> floatAttributes;
};

}

#endif
