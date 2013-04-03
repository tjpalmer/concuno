# Exploration of covariant exploration, and I repeated that on purpose.

include_directories(
  ${Eigen_INCLUDE_DIR}
)

add_executable(
  covary
  covary.cpp
  distributions-covary.cpp
  matrices-covary.cpp
  ${math_LIBRARY}
)

target_link_libraries(
  covary
  ${math_LIBRARY}
)
