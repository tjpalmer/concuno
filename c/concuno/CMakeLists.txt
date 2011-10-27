# TODO Also concuno-dynamic for a shared library. Different names can make
# TODO later linking easier, in my experience.

add_library(concuno-static STATIC
  cluster.c
  core.c
  entity.c
  kdtree.c
  learn.c
  mat.cpp
  io.c
  numpy-mtrand/distributions.c
  numpy-mtrand/initarray.c
  numpy-mtrand/randomkit.c
  search.c
  stats.c
  tree.c
)
