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

  virtual size_t count(const Entity* entity) const = 0;

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
 */
struct Schema {
  Id id;
  std::vector<CategoryAttribute*> categoryAttributes;
  std::vector<FloatAttribute*> floatAttributes;
};

}

#endif
