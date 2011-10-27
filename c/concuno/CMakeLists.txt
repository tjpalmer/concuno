# TODO Also concuno-dynamic for a shared library. Different names can make
# TODO later linking easier, in my experience.

add_library(concuno-static STATIC
  cluster.cpp
  core.cpp
  entity.c
  kdtree.cpp
  learn.c
  mat.cpp
  io.cpp
  numpy-mtrand/distributions.c
  numpy-mtrand/initarray.c
  numpy-mtrand/randomkit.c
  search.cpp
  stats.cpp
  tree.c
)
