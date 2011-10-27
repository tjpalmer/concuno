# TODO Also concuno-dynamic for a shared library. Different names can make
# TODO later linking easier, in my experience.

add_library(concuno-static STATIC
  cluster.cpp
  core.c
  entity.c
  kdtree.c
  learn.c
  mat.cpp
  io.cpp
  numpy-mtrand/distributions.c
  numpy-mtrand/initarray.c
  numpy-mtrand/randomkit.c
  search.cpp
  stats.c
  tree.c
)
