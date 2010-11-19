#ifndef cuncuno_grid_h
#define cuncuno_grid_h

#include "entity.h"

namespace cuncuno {

struct Grid {

  Grid(const Type& type);

  /**
   * Deletes any buffer.
   */
  ~Grid();

  Count count() const;

  const Type& type;

private:

  Count $count;

  /**
   * Being typed, we can short-circuit the need for a separate buffer for small
   * enough types and few enough values.
   */
  union {
    void* buffer;
    Float $float;
    Int $int;
  } data;

};

/**
 * Templatized version for when you do want labeling.
 */
template<typename Item>
struct GridOf: Grid {
  GridOf(const Type& type): Grid(type) {}
};

}

#endif
