include_directories(
  # Clients shouldn't dig directly into these, but cuncuno itself can.
  ${cuncuno_INCLUDE_DIR}/cuncuno
)

add_library(cuncuno SHARED
  entity.cpp
  entity2d3d.cpp
  learner.cpp
  tree.cpp
)

add_executable(cuno cuno.cpp)

target_link_libraries(cuno cuncuno)
