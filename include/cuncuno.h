#ifndef cuncuno_h
#define cuncuno_h

// TODO Use C or C++ as interface????

#ifdef _WIN32
        #ifdef cuno_EXPORTS
                #define cuncunoExport __declspec(dllexport)
        #else
                #define cuncunoExport __declspec(dllimport)
        #endif
        // cunExport becomes export or import.
        // cunModExport is always export and should be used for functions
        //   exported by mods.
        // TODO A system of exports from mods with structs of function pointers.
        // TODO That guarantees keeping the namespace clean.
        // TODO Then each mod could just have a single dllexport function that
        // TODO provides all setup and other exports in the struct.
        #define cuncunoModExport __declspec(dllexport)
#else
        #define cuncunoExport
        #define cuncunoModExport
#endif

#include <string>
#include <vector>

namespace cuncuno {

// TODO C++ here.

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

struct Entity2D: Entity {

  Entity2D();

  Float color[3];

  /**
   * Half sizes, or radii in a sense.
   */
  Float extent[2];

  Float location[2];

  /**
   * Just the angle of rotation.
   */
  Float orientation;

  Float orientationVelocity;

  Float velocity[2];

};

struct Entity3D: Entity {

  Entity3D();

  Float color[3];

  /**
   * Half sizes, or radii in a sense.
   */
  Float extent[3];

  Float location[3];

  /**
   * As a quaternion, or use axis-angle? If axis-angle, normalize to 3 vals?
   */
  Float orientation[4];

  /**
   * As a quaternion, or use axis-angle? If axis-angle, normalize to 3 vals?
   */
  Float orientationVelocity[4];

  Float velocity[3];

};

/**
 * Represents a data sample for training or testing.
 *
 * Ideally, this will support both classification and multidimensional
 * regression.
 */
template<typename Value>
struct Sample {

  /**
   * TODO Some kind of argument identification among the entities in the bag.
   * TODO SMRF calls that h-pinning.
   */
  std::vector<const Entity*> entities;

  /**
   * TODO We need to be able to associate this with some kind of entity
   * TODO attribute. Sometimes, though, disconnected values will also be of
   * TODO use.
   */
  Value value;

};

typedef Sample<bool> BoolSample;

/**
 * TODO What kind of return type?
 * TODO What should this be named?
 * TODO How to provide a schema?
 */
void learn(const std::vector<BoolSample>& samples);

}

#ifdef __cplusplus
extern "C" {
#endif

// TODO C here.

#ifdef __cplusplus
}
#endif

#endif
