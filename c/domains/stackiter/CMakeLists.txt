add_executable(
  stackiter-learn
  stackiter-learn.c
  choose.c
  load.c
  state.c
)

target_link_libraries(
  stackiter-learn
  concuno-static
  #${blas_LIBRARY}
  #${cblas_LIBRARY}
  ${math_LIBRARY}
)
