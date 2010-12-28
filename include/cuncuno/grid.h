#ifndef cuncuno_grid_h
#define cuncuno_grid_h

#include "entity.h"

namespace cuncuno {


struct List {

  List(const Type& type);

  List(const Type& type, Count count);

  /**
   * Deletes the buffer if owned by this.
   */
  ~List();

  Count count() const;

  /**
   * TODO Or require fixed size?
   */
  void push(const List& items);

  const Type& type;

private:

  /**
   * Negative to indicate the buffer is owned by someone else.
   */
  Int $count;

  /**
   * Being typed, we can short-circuit the need for a separate buffer for small
   * enough types and few enough values.
   */
  union {
    void* buffer;
    Count count;
    Float $float;
    Int $int;
  } data;

};


template<typename Item>
struct ListOf: List {

  ListOf(const Type& type): List(type) {}

  ListOf(const Type& type, Count count): List(type, count) {}

  Item& operator[](Count index);

};


template<typename Item>
struct GridOf;


struct Grid {

  Grid(const Type& type);

  Grid(const Type& type, Count ndim, ...);

  /**
   * Argument count must equal ndim, else expect crashing.
   */
  void* at(Count index0, ...);

  Count count() const;

  Count ndim() const;

  template<typename Item>
  GridOf<Item>& of() {
    return *reinterpret_cast<GridOf<Item>*>(this);
  }

  GridOf<Float>& of() {
    if (type() != type().system.$float()) {
      throw "not of float";
    }
    return *reinterpret_cast<GridOf<Float>*>(this);
  }

  void push(const Grid& grid, Count dim);

  /**
   * Resize along a particular dimension.
   */
  void resize(Count dim, Count size);

  void shape(GridOf<Count>& out) const;

  Count size(Count dim) const;

  const Type& type() const;

private:

  /**
   * The final size does not need indicated, so for 2D (matrices), no separate
   * storage should be needed for the shape.
   */
  ListOf<Count> $shape;

  List list;

};


/**
 * Templatized version for when you do want labeling.
 */
template<typename Item>
struct GridOf: Grid {
  GridOf(const Type& type): Grid(type) {}
};


template<typename Item>
ListOf<Item>& operator,(ListOf<Item>& list, const Item& item);


void min(GridOf<Float>& out, const GridOf<Float>& in, Count dim);


}

#endif
