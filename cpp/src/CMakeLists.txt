include_directories(
  # Clients shouldn't dig directly into these, but cuncuno itself can.
  ${cuncuno_INCLUDE_DIR}/cuncuno
)

add_library(cuncuno SHARED
  context.cpp
  entity.cpp
  entity2d3d.cpp
  functions.cpp
  grid.cpp
  learner.cpp
  predicate.cpp
  tree.cpp
)

add_executable(cuno cuno.cpp)

target_link_libraries(cuno cuncuno)
