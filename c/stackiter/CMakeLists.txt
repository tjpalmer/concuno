add_executable(
  stackiter-learner
  stackiter-learner.c
  chooser.c
  loader.c
  state.c
)

target_link_libraries(
  stackiter-learner
  cuncuno
  ${blas_LIBRARY}
  ${cblas_LIBRARY}
  ${math_LIBRARY}
)
