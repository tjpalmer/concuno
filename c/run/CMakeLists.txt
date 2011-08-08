add_executable(
  concuno-run
  run.c
)

target_link_libraries(
  concuno-run
  concuno-static
  ${math_LIBRARY}
)
