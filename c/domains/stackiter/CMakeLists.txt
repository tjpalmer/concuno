add_executable(
  stackiter-learn
  stackiter-learn.cpp
  choose.cpp
  load.cpp
  state.cpp
)

target_link_libraries(
  stackiter-learn
  concuno-static
  #${blas_LIBRARY}
  #${cblas_LIBRARY}
  ${math_LIBRARY}
)
